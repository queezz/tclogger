export function renderPlot(container, series, opts={}){
  if(!container) return;
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
  ctx.strokeStyle='rgba(255,255,255,0.08)'; ctx.lineWidth=1; ctx.beginPath(); ctx.moveTo(mL,mT); ctx.lineTo(mL,h-mB); ctx.lineTo(w-mR,h-mB); ctx.stroke();
  ctx.strokeStyle='rgba(79,209,197,0.98)'; ctx.lineWidth=2; ctx.beginPath();
  for(let i=0;i<y.length;i++){
    const xi = mL + ((x[i]-xmin)/(xmax-xmin||1))*(w-mL-mR);
    const yi = mT + ((y1-y[i])/(y1-y0||1))*(h-mT-mB);
    if(i===0) ctx.moveTo(xi,yi); else ctx.lineTo(xi,yi);
  }
  ctx.stroke();
}
function drawNoData(ctx,w,h){ ctx.fillStyle='rgba(230,238,246,0.75)'; ctx.font='12px system-ui'; ctx.fillText('No data', 12, 18); ctx.strokeStyle='rgba(255,255,255,0.08)'; ctx.strokeRect(8,8,w-16,h-16); }
