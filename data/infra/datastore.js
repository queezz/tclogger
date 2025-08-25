import { fetchFileList, fetchFileBlob } from './api.js';
import { getAllFiles, putFileMeta, getBlob, putBlob, getSeries, putSeries } from './cache.js';

let worker;
function getWorker(){ if(!worker){ const url = new URL('./parser.worker.js', import.meta.url); worker = new Worker(url, { type: 'module' }); } return worker; }

export async function getFileListCached(){ const list = await getAllFiles(); return list.sort((a,b)=> (b.lastModified??0)-(a.lastModified??0) || String(b.name).localeCompare(String(a.name))); }

export async function refreshFileList(paging){ const remote = await fetchFileList(paging); for(const m of remote.files){ await putFileMeta(m); } return remote; }

export async function ensureSeries(name){ const cached = await getSeries(name); if(cached) return { series: cached.series, meta: cached.meta, fromCache: true };
  let blob = await getBlob(name); if(!blob){ blob = await fetchFileBlob(name); await putBlob(name, blob); }
  const parsed = await parseInWorker(name, blob);
  await putSeries(name, parsed.series, parsed.meta);
  return { series: parsed.series, meta: parsed.meta, fromCache: false };
}

function parseInWorker(name, blob){ return new Promise((resolve, reject)=>{
  const w = getWorker();
  const onMsg = (e)=>{ const d=e.data; if(d && d.ok && d.name===name){ w.removeEventListener('message', onMsg); resolve(d); } };
  const onErr = (e)=>{ w.removeEventListener('message', onMsg); reject(e.error||new Error('worker error')); };
  w.addEventListener('message', onMsg);
  w.addEventListener('error', onErr, { once:true });
  w.postMessage({ type:'parse-csv', name, options:{ skipHeaderLines:3 }, blob });
  setTimeout(()=>{ try{ w.removeEventListener('message', onMsg); }catch(_){} reject(new Error('parse timeout')); }, 20000);
}); }
