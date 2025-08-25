self.onmessage = async (e)=>{
  const { type } = e.data || {};
  if(type === 'parse-csv'){
    const { name, blob, options } = e.data;
    try{
      const text = await blob.text();
      const lines = text.split(/\r?\n/);
      const skip = Number(options?.skipHeaderLines ?? 3);
      let delim = options?.delimiter;
      // detect delimiter from first non-empty line after skip
      let start = 0; let nonEmpty = 0; for(let i=0;i<lines.length;i++){ if(lines[i].trim().length){ nonEmpty++; if(nonEmpty===1){ start = i; break; } } }
      if(!delim){ const first = (lines[start]||''); delim = (first.indexOf(';')>=0 && first.indexOf(',')<0) ? ';' : ','; }
      const x=[]; const y=[];
      for(let i=0;i<lines.length;i++){
        const raw = lines[i]; if(!raw) continue; if(i < skip) continue; const s = raw.trim(); if(!s || s.startsWith('#')) continue;
        const parts = s.split(delim);
        if(parts.length < 2) continue;
        const tRaw = parts[0].trim(); const vRaw = parts[1].trim();
        const v = Number(vRaw); if(!Number.isFinite(v)) continue;
        let ts = Number(tRaw);
        if(Number.isFinite(ts)){
          // interpret as seconds if looks like epoch seconds, otherwise ms
          if(ts < 1e11) ts = ts * 1000;
        } else {
          const dt = Date.parse(tRaw); if(Number.isFinite(dt)) ts = dt; else continue;
        }
        x.push(ts); y.push(v);
      }
      const n = x.length; const meta = n ? { n, t0:x[0], t1:x[n-1], ymin:Math.min(...y), ymax:Math.max(...y) } : { n:0 };
      self.postMessage({ ok:true, name, series:{ x, y }, meta });
    }catch(err){ self.postMessage({ ok:false, name, error: String(err?.message||err) }); }
  }
};
