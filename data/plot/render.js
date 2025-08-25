const seriesStore = new WeakMap();

export function renderPlot(container, series, opts={}){
  if(!container) return;
  seriesStore.set(container, { base: cloneSeries(series) });
  draw(container, series, opts);
}

export function updatePlot(container, opts={}){
  if(!container) return;
  const stored = seriesStore.get(container);
  if(!stored || !stored.base) return;
  const processed = applyTransforms(stored.base, opts);
  draw(container, processed, opts);
}

function draw(container, series, opts){
  let canvas = container.querySelector('canvas');
  if(!canvas){ canvas = document.createElement('canvas'); canvas.style.width='100%'; canvas.style.height='100%'; container.appendChild(canvas); }
  const ctx = canvas.getContext('2d'); const dpr = window.devicePixelRatio||1;
  const w = container.clientWidth || 800; const h = container.clientHeight || 300;
  canvas.width = Math.max(1, Math.floor(w*dpr)); canvas.height = Math.max(1, Math.floor(h*dpr)); ctx.setTransform(dpr,0,0,dpr,0,0); ctx.clearRect(0,0,w,h);
  const x = series?.x || []; const y = series?.y || []; if(x.length<2 || y.length<2){ drawNoData(ctx,w,h); return; }
  const mL=60,mR=16,mT=16,mB=40;
  const xmin = Math.min(...x), xmax = Math.max(...x);
  const valid = y.filter(v=>Number.isFinite(v)); if(valid.length===0){ drawNoData(ctx,w,h); return; }
  const ymin = Math.min(...valid), ymax = Math.max(...valid); const pad=(ymax-ymin||1)*0.12; const y0=ymin-pad, y1=ymax+pad;
  // grid + axes box
  ctx.strokeStyle='rgba(255,255,255,0.08)'; ctx.lineWidth=1; ctx.beginPath(); ctx.moveTo(mL,mT); ctx.lineTo(mL,h-mB); ctx.lineTo(w-mR,h-mB); ctx.stroke();
  // y ticks and labels
  ctx.fillStyle='rgba(230,238,246,0.95)'; ctx.font='12px system-ui'; ctx.textAlign='right'; ctx.textBaseline='middle';
  const yTicks = 5; for(let i=0;i<=yTicks;i++){ const v = y0 + (i / yTicks) * (y1 - y0); const yy = mT + ((y1 - v)/(y1 - y0)) * (h - mT - mB); ctx.beginPath(); ctx.moveTo(mL-6, yy); ctx.lineTo(mL, yy); ctx.stroke(); ctx.fillText(v.toFixed(2), mL - 8, yy); }
  // x ticks and labels
  ctx.textAlign='center'; ctx.textBaseline='top'; const xTicks = 6; for(let i=0;i<=xTicks;i++){ const t = xmin + (i / xTicks) * (xmax - xmin); const xx = mL + ((t - xmin)/(xmax - xmin||1))*(w - mL - mR); ctx.beginPath(); ctx.moveTo(xx, h - mB); ctx.lineTo(xx, h - mB + 6); ctx.stroke(); const dt = new Date(t); const label = dt.toLocaleTimeString(undefined, { hour:'2-digit', minute:'2-digit', second:'2-digit', hour12:false }); ctx.fillText(label, xx, h - mB + 8); }
  // series line
  ctx.strokeStyle='rgba(79,209,197,0.98)'; ctx.lineWidth=2; ctx.beginPath();
  for(let i=0;i<y.length;i++){
    const xi = mL + ((x[i]-xmin)/(xmax-xmin||1))*(w-mL-mR);
    const yi = mT + ((y1-y[i])/(y1-y0||1))*(h-mT-mB);
    if(i===0) ctx.moveTo(xi,yi); else ctx.lineTo(xi,yi);
  }
  ctx.stroke();
  // axis labels
  ctx.fillStyle='rgba(154,166,178,0.95)'; ctx.font='13px system-ui'; ctx.textAlign='center'; ctx.fillText('Time', mL + (w - mL - mR)/2, h - Math.floor(mB/2) + 6);
  ctx.save(); ctx.translate(mL - 50, mT + Math.floor((h - mT - mB)/2)); ctx.rotate(-Math.PI/2); ctx.textAlign='center'; ctx.fillText('Temperature', 0, 0); ctx.restore();
}

function drawNoData(ctx,w,h){ ctx.fillStyle='rgba(230,238,246,0.75)'; ctx.font='12px system-ui'; ctx.fillText('No data', 12, 18); ctx.strokeStyle='rgba(255,255,255,0.08)'; ctx.strokeRect(8,8,w-16,h-16); }

function cloneSeries(s){ return { x: s?.x ? s.x.slice() : [], y: s?.y ? s.y.slice() : [] }; }

function applyTransforms(series, opts={}){
  const x = series.x, y = series.y; let idx = [...x.keys()];
  const hasT0 = Number.isFinite(opts?.t0); const hasT1 = Number.isFinite(opts?.t1);
  if(hasT0 || hasT1){ const t0 = hasT0? Number(opts.t0) : -Infinity; const t1 = hasT1? Number(opts.t1) : Infinity; idx = idx.filter(i=> x[i] >= t0 && x[i] <= t1); }
  const stride = Math.max(1, Number(opts?.decimateStride||1)); if(stride>1){ idx = idx.filter((_,i)=> (i % stride) === 0); }
  const smoothN = Math.max(0, Number(opts?.smoothN||0));
  const smoothMs = Math.max(0, Number(opts?.smoothMs||0));
  let outX = idx.map(i=> x[i]); let outY = idx.map(i=> y[i]);
  if(smoothN > 1){ outY = movingAverage(outY, smoothN); }
  else if(smoothMs > 1){ outY = movingAverageByTime(outX, outY, smoothMs); }
  const target = Math.max(0, Number(opts?.targetPoints||0));
  if(target && outX.length > target){ const step = Math.ceil(outX.length / target); const nn=[]; const ny=[]; for(let i=0;i<outX.length;i+=step){ nn.push(outX[i]); ny.push(outY[i]); } outX = nn; outY = ny; }
  return { x: outX, y: outY };
}

function movingAverage(arr, n){ if(n<=1) return arr.slice(); const out=new Array(arr.length); let sum=0; for(let i=0;i<arr.length;i++){ sum += arr[i]; if(i>=n) sum -= arr[i-n]; out[i] = sum / Math.min(i+1, n); } return out; }

function movingAverageByTime(x, y, windowMs){ if(windowMs<=1) return y.slice(); const out=new Array(y.length); let j=0; let sum=0; for(let i=0;i<y.length;i++){ sum += y[i]; while(x[i]-x[j] > windowMs && j<i){ sum -= y[j]; j++; } const count = i - j + 1; out[i] = sum / count; } return out; }
