import { getFileListCached, refreshFileList, ensureSeries } from './infra/datastore.js';
import { renderPlot } from './plot/render.js';

const el = {
  fileSelect: document.getElementById('fileSelect'),
  loadBtn: document.getElementById('loadBtn'),
  decimate: document.getElementById('decimate'),
  avgWindow: document.getElementById('avgWindow'),
  info: document.getElementById('info'),
  savePng: document.getElementById('savePng'),
  canvasWrap: document.getElementById('canvasWrap')
};

function setInfo(t){ if(el.info) el.info.textContent = t; }

async function init(){ await populateFiles(); await refreshFiles(); bind(); await autoFromQuery(); }

async function populateFiles(){ const cached = await getFileListCached(); fillSelect(cached); }
async function refreshFiles(){ try{ const r = await refreshFileList({ page:1, pageSize:50 }); fillSelect((await getFileListCached())); }catch(e){} }

function fillSelect(list){ if(!el.fileSelect) return; el.fileSelect.innerHTML=''; if(!list || list.length===0){ el.fileSelect.innerHTML='<option>No files</option>'; return; } list.forEach(f=>{ const o=document.createElement('option'); o.value=f.name; o.textContent=`${f.name} ${f.size? '('+Math.round(f.size/1024)+'KB)':''}`; el.fileSelect.appendChild(o); }); }

function bind(){ if(el.loadBtn) el.loadBtn.addEventListener('click', onLoad);
  if(el.savePng) el.savePng.addEventListener('click', onSavePng);
}

async function onLoad(){ const name = el.fileSelect?.value; if(!name) return; setInfo('Loading '+name+'…'); const { series } = await ensureSeries(name); draw(series, name); setInfo(`Plotted ${series.x.length} points`); }

function draw(series, name){ renderPlot(el.canvasWrap, series, { title:name }); }

function onSavePng(){ const c = el.canvasWrap?.querySelector('canvas'); if(!c) return; const url = c.toDataURL('image/png'); const a=document.createElement('a'); a.href=url; a.download='plot.png'; a.click(); }

async function autoFromQuery(){ const params = new URLSearchParams(location.search); const file = params.get('file'); if(file){ let found=false; for(const o of el.fileSelect.options){ if(o.value===file){ el.fileSelect.value=file; found=true; break; } } if(!found){ const opt=document.createElement('option'); opt.value=file; opt.textContent=file; el.fileSelect.appendChild(opt); el.fileSelect.value=file; } await onLoad(); }
}

window.addEventListener('DOMContentLoaded', init);
