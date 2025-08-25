import { getFileListCached, refreshFileList, ensureSeries } from './infra/datastore.js';
import { renderPlot, updatePlot } from './plot/render.js';

const el = {
  fileSelect: document.getElementById('fileSelect'),
  loadBtn: document.getElementById('loadBtn'),
  info: document.getElementById('info'),
  plot: document.getElementById('canvasWrap') || document.getElementById('plot'),
  smoothN: document.getElementById('smoothN'),
  smoothMs: document.getElementById('smoothMs'),
  highRes: document.getElementById('highRes'),
  range6h: document.getElementById('range6h'),
  rangeAll: document.getElementById('rangeAll'),
  savePng: document.getElementById('savePng')
};

let CURRENT = { series: null, opts: { highRes:false, smoothN:0, smoothMs:0, t0:null, t1:null } };

function setInfo(t){ if(el.info) el.info.textContent = t; }

async function init(){ await populateFiles(); await refreshFiles(); bind(); await autoFromQuery(); }

async function populateFiles(){ const cached = await getFileListCached(); fillSelect(cached); }
async function refreshFiles(){ try{ await refreshFileList({ page:1, pageSize:50 }); fillSelect((await getFileListCached())); }catch(e){} }

function fillSelect(list){ if(!el.fileSelect) return; el.fileSelect.innerHTML=''; if(!list || list.length===0){ el.fileSelect.innerHTML='<option>No files</option>'; return; } list.forEach(f=>{ const o=document.createElement('option'); o.value=f.name; o.textContent=`${f.name} ${f.size? '('+Math.round(f.size/1024)+'KB)':''}`; el.fileSelect.appendChild(o); }); }

function bind(){ if(el.loadBtn) el.loadBtn.addEventListener('click', onLoad);
  if(el.savePng) el.savePng.addEventListener('click', onSavePng);
  el.smoothN?.addEventListener('input', ()=>{ CURRENT.opts.smoothN = Number(el.smoothN.value)||0; CURRENT.opts.smoothMs = 0; updatePlot(el.plot, CURRENT.opts); });
  el.smoothMs?.addEventListener('input', ()=>{ CURRENT.opts.smoothMs = Number(el.smoothMs.value)||0; CURRENT.opts.smoothN = 0; updatePlot(el.plot, CURRENT.opts); });
  el.highRes?.addEventListener('change', ()=>{ CURRENT.opts.highRes = !!el.highRes.checked; updatePlot(el.plot, CURRENT.opts); });
  el.range6h?.addEventListener('click', ()=>{ const t1=Date.now(); const t0=t1-6*3600*1000; CURRENT.opts.t0=t0; CURRENT.opts.t1=t1; updatePlot(el.plot, CURRENT.opts); });
  el.rangeAll?.addEventListener('click', ()=>{ CURRENT.opts.t0=null; CURRENT.opts.t1=null; updatePlot(el.plot, CURRENT.opts); });
}

async function onLoad(){ const name = el.fileSelect?.value; if(!name) return; setInfo('Loading '+name+'…'); const { series } = await ensureSeries(name); CURRENT.series = series; renderPlot(el.plot, series, CURRENT.opts); setInfo(`Plotted ${series.x.length} points`); }

function onSavePng(){ const c = el.plot?.querySelector('canvas'); if(!c) return; const url = c.toDataURL('image/png'); const a=document.createElement('a'); a.href=url; a.download='plot.png'; a.click(); }

async function autoFromQuery(){ const params = new URLSearchParams(location.search); const file = params.get('file'); if(file){ let found=false; for(const o of el.fileSelect.options){ if(o.value===file){ el.fileSelect.value=file; found=true; break; } } if(!found){ const opt=document.createElement('option'); opt.value=file; opt.textContent=file; el.fileSelect.appendChild(opt); el.fileSelect.value=file; } await onLoad(); }
}

window.addEventListener('DOMContentLoaded', init);
