const API_LIST = '/files.json';
const LOG_PREFIX = '/logs/';

export async function fetchFileList({ page=1, pageSize=10 }={}){
  // Try server-side paging first
  try{
    const r = await fetch(`${API_LIST}?page=${page}&page_size=${pageSize}`);
    if(r.ok){ const d = await r.json(); if(Array.isArray(d.items) && (typeof d.total !== 'undefined')) return { files: d.items.map(mapFile), total: d.total } }
  }catch(e){}
  // Fallback: load all and paginate client-side
  const r2 = await fetch(API_LIST);
  const d2 = await r2.json();
  const items = Array.isArray(d2.items) ? d2.items : Array.isArray(d2) ? d2 : [];
  items.sort((a,b)=> (new Date(b.mtime||0)) - (new Date(a.mtime||0)) || String(b.name).localeCompare(String(a.name)) );
  return { files: items.slice((page-1)*pageSize, page*pageSize).map(mapFile), total: items.length };
}

export async function fetchFileBlob(name){
  const r = await fetch(LOG_PREFIX + encodeURIComponent(name));
  if(!r.ok) throw new Error('http '+r.status);
  return await r.blob();
}

function mapFile(f){ return { name: String(f.name).split('/').pop(), size: Number(f.size||0), lastModified: f.mtime? Number(new Date(f.mtime)) : undefined } }
