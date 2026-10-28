"""Run the frozen C++ dependency predictor on eight complete source profiles."""
import argparse, hashlib, json, os, shutil, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from reproduce import load, verify_manifest

def native_path(path):
    value=str(Path(path).resolve()).replace(chr(92),'/')
    if os.name=='nt':return '/mnt/'+value[0].lower()+'/'+value[3:]
    return value
def command(args,**kwargs):
    if os.name=='nt':args=['wsl.exe','--distribution',os.environ.get('PACERV_WSL_DISTRO','Ubuntu-22.04'),'--exec']+args
    subprocess.run(args,check=True,**kwargs)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--limit',type=int);args=ap.parse_args()
    verify_manifest();out=ROOT/'output/replay';out.mkdir(parents=True,exist_ok=True)
    exe=out/'graph_forecast'
    command(['g++','-std=c++20','-O3',native_path(ROOT/'src/graph_forecast.cpp'),'-lz','-o',native_path(exe)])
    checked=[]
    for i,row in enumerate(load('data/replay/cases.json')):
        if args.limit is not None and i>=args.limit:break
        source=ROOT/'data/replay'/f'E{row["E"]}'/row['workload'];dest=out/row['service']/f'E{row["E"]}'/row['workload'];dest.mkdir(parents=True,exist_ok=True)
        csv=dest/(row['model']+'.csv');summary=dest/(row['model']+'.json')
        flags=['1' if row['model'].startswith('repaired') else '0','1' if row['model'].endswith('relative') else '0']
        argv=[native_path(exe),native_path(source/'graph.gz'),native_path(source/'commands.csv'),native_path(source/'cancels.csv'),
              native_path(csv),native_path(summary),str(row['nodes']),'32767','new' if row['service']=='bit67' else 'bit89',
              native_path(source/'prepared.csv'),native_path(source/'sparse.csv')]+flags
        command(argv,stdout=subprocess.DEVNULL)
        actual=hashlib.sha256(csv.read_bytes()).hexdigest()
        if actual!=row['output_sha256']:raise ValueError('Forecast differs: '+str(row))
        checked.append(dict(service=row['service'],E=row['E'],workload=row['workload'],model=row['model'],sha256=actual))
    (out/'verification.json').write_text(json.dumps({'status':'passed','cases':checked},indent=2)+'\n',encoding='utf-8')
    print(f'PASS: {len(checked)} complete-profile forecasts match frozen outputs byte for byte')

if __name__=='__main__':main()
