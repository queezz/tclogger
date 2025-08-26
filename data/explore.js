import { getFileListCached, refreshFileList, ensureSeries } from './infra/datastore.js';
import { renderPlot, updatePlot } from './plot/render.js';
import { fetchFileBlob } from './infra/api.js';
import { getBlob, putBlob } from './infra/cache.js';

const PAGE_SIZE_KEY = 'tcl.pageSize';
let page = 1; let pageSize = Number(localStorage.getItem(PAGE_SIZE_KEY) || 10); let total = 0; let currentName = null;
let tableMinimized = false;

async function init(){
  await renderFromCache();
  try{ const remote = await refreshFileList({ page, pageSize }); total = remote.total||0; await renderFromCache(); }catch(e){ console.warn('refresh list failed', e); }
  bindPager();
  bindInlineControls();
}

// Use actual cached list length for paging to avoid phantom empty pages
async function renderFromCache(){
  const list = await getFileListCached();
  total = list.length;
  const totalPages = Math.max(1, Math.ceil(total / pageSize));
  if(page > totalPages) page = totalPages; if(page < 1) page = 1;
  const start = (page - 1) * pageSize; const end = start + pageSize;
  const pageRows = list.slice(start, end);
  renderTable(pageRows);
  // Maintain minimize/show button state in sync with rendered rows.
  if(tableMinimized){
    const exists = !!document.querySelector(`#logsBody tr[data-name="${cssEscape(currentName)}"]`);
    if(exists){
      // Keep table minimized to the current item and ensure button label reflects that
      minimizeTableToCurrent(currentName);
      const btn = document.getElementById('showAllBtn'); if(btn) btn.textContent = 'Show all files';
    } else {
      // Current item not on this page: reset minimized state and update label
      tableMinimized = false;
      const btn = document.getElementById('showAllBtn'); if(btn) btn.textContent = 'Minimize table';
    }
  } else {
    const btn = document.getElementById('showAllBtn'); if(btn) btn.textContent = 'Minimize table';
  }
  renderPager(totalPages);
}

function renderTable(items){ const tb = document.getElementById('logsBody'); if(!tb) return; tb.innerHTML=''; items.forEach(f=>{ const tr=document.createElement('tr'); const name=String(f.name||''); const sizeKB=Math.round((f.size||0)/1024);
  tr.dataset.name = name;
  tr.innerHTML = `
    <td>${name}</td>
    <td>${sizeKB} KB</td>
    <td class="actions">
      <button class="plot" data-name="${name}">Plot</button>
      <button class="dl" data-name="${name}">Download</button>
      <span class="spinner hidden" aria-hidden="true"></span>
    </td>`; tb.appendChild(tr); });
  tb.querySelectorAll('button.plot').forEach(btn=> btn.addEventListener('click', async ()=>{ await onPlot(btn.dataset.name); }));
  tb.querySelectorAll('button.dl').forEach(btn=> btn.addEventListener('click', async ()=>{ await onDownload(btn.dataset.name); }));
}

function renderPager(pages){ const pager = document.getElementById('pager'); pages ||= Math.max(1, Math.ceil(total / pageSize)); pager.innerHTML = `
  <button ${page<=1?'disabled':''} id="prevBtn">« Prev</button>
  <span>Page ${page} of ${pages}</span>
  <button ${page>=pages?'disabled':''} id="nextBtn">Next »</button>`;
  pager.querySelector('#prevBtn')?.addEventListener('click', ()=>{ if(page>1){ page--; renderFromCache(); }});
  pager.querySelector('#nextBtn')?.addEventListener('click', ()=>{ if(page<pages){ page++; renderFromCache(); }});
}

function bindPager(){ /* noop for now */ }

function bindInlineControls(){ const s = document.getElementById('smoothNInline'); const showAll = document.getElementById('showAllBtn'); if(s){ s.addEventListener('input', ()=>{ const n = Math.max(0, Number(s.value)||0); updatePlot(document.getElementById('canvasWrap'), { smoothN:n, smoothMs:0 }); }); }
  if(showAll){
    // initialize label
    showAll.textContent = tableMinimized ? 'Show all files' : 'Minimize table';
    showAll.addEventListener('click', async ()=>{
      tableMinimized = !tableMinimized;
      if(tableMinimized){ // hide other rows, keep current visible
        if(currentName) minimizeTableToCurrent(currentName);
        showAll.textContent = 'Show all files';
      } else {
        showAll.textContent = 'Minimize table';
        showAllRows();
      }
    });
  }
}

async function onPlot(name){ try{
  currentName = name; const card = document.getElementById('inlinePlotCard'); const wrap = document.getElementById('canvasWrap'); const info = document.getElementById('plotInfo'); if(!card||!wrap||!info) return;
  info.textContent = `Loading ${name}…`; card.classList.remove('hidden');
  // show spinner in the table row
  const row = document.querySelector(`#logsBody tr[data-name="${cssEscape(name)}"]`);
  const spinner = row?.querySelector('.spinner'); if(spinner) spinner.classList.remove('hidden');
  // do not auto-hide files; user controls minimization via the toggle
  const { series } = await ensureSeries(name);
  info.textContent = `Preview: ${name} (${series.x.length} points)`;
  renderPlot(wrap, series, { targetPoints: 2000 });
  if(spinner) spinner.classList.add('hidden');
}catch(e){ console.error(e); const info = document.getElementById('plotInfo'); if(info) info.textContent = 'No data'; }}

function minimizeTableToCurrent(name){ const rows = document.querySelectorAll('#logsBody tr'); rows.forEach(r=>{ r.style.display = (r.dataset.name===name) ? '' : 'none'; }); }

function showAllRows(){ const rows = document.querySelectorAll('#logsBody tr'); rows.forEach(r=>{ r.style.display = ''; }); }

// helper to escape CSS attribute selector characters
function cssEscape(s){ return s.replace(/(["\\])/g,'\\$1'); }

async function onDownload(name){ let blob = await getBlob(name); if(!blob){ blob = await fetchFileBlob(name); await putBlob(name, blob); } const a=document.createElement('a'); a.href=URL.createObjectURL(blob); a.download=name; a.click(); URL.revokeObjectURL(a.href); }

window.addEventListener('DOMContentLoaded', init);
