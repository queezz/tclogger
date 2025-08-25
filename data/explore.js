import { getFileListCached, refreshFileList, ensureSeries } from './infra/datastore.js';
import { renderPlot } from './plot/render.js';

const PAGE_SIZE_KEY = 'tcl.pageSize';
let page = 1; let pageSize = Number(localStorage.getItem(PAGE_SIZE_KEY) || 10); let total = 0; let currentName = null;

async function init(){
  await renderFromCache();
  try{ const remote = await refreshFileList({ page, pageSize }); total = remote.total||0; await renderFromCache(); }catch(e){ console.warn('refresh list failed', e); }
  bindPager();
}

async function renderFromCache(){ const list = await getFileListCached(); total = total || list.length; renderTable(list.slice((page-1)*pageSize, page*pageSize)); renderPager(); }

function renderTable(items){ const tb = document.getElementById('logsBody'); if(!tb) return; tb.innerHTML=''; items.forEach(f=>{ const tr=document.createElement('tr'); const name=String(f.name||''); const sizeKB=Math.round((f.size||0)/1024);
  const mtime=f.lastModified? new Date(f.lastModified).toLocaleString(): '-';
  tr.innerHTML = `
    <td>${name}</td>
    <td>${sizeKB} KB</td>
    <td>${mtime}</td>
    <td class="actions">
      <button class="plot" data-name="${name}">Plot</button>
      <a class="button" href="/logs/${encodeURIComponent(name)}" download>Download</a>
      <a class="button" href="/plotter.html?file=${encodeURIComponent(name)}" target="_blank">Open in Plotter</a>
    </td>`; tb.appendChild(tr); });
  tb.querySelectorAll('button.plot').forEach(btn=> btn.addEventListener('click', async ()=>{ await onPlot(btn.dataset.name); }));
}

function renderPager(){ const pager = document.getElementById('pager'); const pages = Math.max(1, Math.ceil(total / pageSize)); pager.innerHTML = `
  <button ${page<=1?'disabled':''} id="prevBtn">« Prev</button>
  <span>Page ${page} of ${pages}</span>
  <button ${page>=pages?'disabled':''} id="nextBtn">Next »</button>`;
  pager.querySelector('#prevBtn')?.addEventListener('click', ()=>{ if(page>1){ page--; renderFromCache(); }});
  pager.querySelector('#nextBtn')?.addEventListener('click', ()=>{ if(page<pages){ page++; renderFromCache(); }});
}

function bindPager(){ const pager = document.getElementById('pager'); if(!pager) return; }

async function onPlot(name){ try{
  currentName = name; const card = document.getElementById('inlinePlotCard'); const wrap = document.getElementById('canvasWrap'); const info = document.getElementById('plotInfo'); if(!card||!wrap||!info) return;
  info.textContent = `Loading ${name}…`; card.classList.remove('hidden');
  const { series } = await ensureSeries(name); // await ensures data ready
  info.textContent = `Preview: ${name} (${series.x.length} points)`;
  renderPlot(wrap, series, { title: name });
}catch(e){ console.error(e); const info = document.getElementById('plotInfo'); if(info) info.textContent = 'No data'; }}

window.addEventListener('DOMContentLoaded', init);
