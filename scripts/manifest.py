"""Generate the tracked-file manifest before an artifact release."""
from pathlib import Path
import hashlib, json
ROOT=Path(__file__).resolve().parents[1]
def main():
    rows=[]
    for p in sorted(ROOT.rglob('*')):
        rel=p.relative_to(ROOT)
        if not p.is_file() or any(part in ('.git','output','__pycache__','.venv','external') for part in rel.parts) or p.name=='FILE_MANIFEST.json':continue
        rows.append(dict(path=rel.as_posix(),bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest()))
    (ROOT/'FILE_MANIFEST.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    print(f'Manifest: {len(rows)} files')
if __name__=='__main__':main()
