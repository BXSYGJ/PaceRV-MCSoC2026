"""Rerun timed native-memory target RTL with separately installed PDS models."""
import argparse, json, os, shutil, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from reproduce import load, require, sha, verify_manifest
from replay import command, native_path

VENDOR_HASHES={'GTP_DRM36K_E1.v':'3e372cd60025a439eb7435d770930260e22c40f148ae83c4bdf7398fb65ddb66',
               'GTP_GRS.v':'dae720bcc467ddb4ff99f2753368934e2649b87f6ac63b8a0d7219699bac1dec'}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--vendor',type=Path,required=True,help='Directory containing the two original PDS simulation models')
    ap.add_argument('--verilator',default='verilator',help='Verilator executable, native Linux path when using WSL')
    ap.add_argument('--case',default='bit67/E0/K2/riscv-matmul16');ap.add_argument('--all',action='store_true')
    ap.add_argument('--jobs',type=int,default=2);args=ap.parse_args()
    require(args.jobs>0,'Positive build-job count required');verify_manifest()
    for name,digest in VENDOR_HASHES.items():require(sha(args.vendor/name)==digest,'Vendor model revision differs: '+name)
    allcases=load('data/unseen/rtl-cases.json');selected=allcases if args.all else [r for r in allcases if r['case']==args.case]
    require(bool(selected),'Unknown case; see data/unseen/rtl-cases.json')
    configs=load('src/rtl/models.json');root=ROOT/'output/rtl';root.mkdir(parents=True,exist_ok=True)
    results=[];built={}
    for row in selected:
        key=(row['service'],row['E'],row['K'])
        if key not in built:
            config=next(c for c in configs if c['service']==row['service'] and c['K']==row['K'])
            source=ROOT/config['directory'];build=root/'models'/row['service']/f'E{row["E"]}'/('Kinf' if row['K']==0 else f'K{row["K"]}')
            build.mkdir(parents=True,exist_ok=True);objects=build/'objects';log=build/'build.log'
            params=[f'-GEARLY_D_REQUEST={row["E"]}' if p.startswith('-GEARLY_D_REQUEST=') else p for p in config['parameters']]
            files=[source/p for p in config['rtl']]+[source/'sim/passive_study_observer.sv']+[args.vendor/n for n in VENDOR_HASHES]+[source/'sim/native_driver.cpp']
            argv=[args.verilator,'--cc','--exe','--build','--timing','--Wall','-Wno-fatal','-j',str(args.jobs),
                  '-MAKEFLAGS','OPT_FAST=-O3','--top-module','rv32_observed_soc','--Mdir',native_path(objects),
                  '-CFLAGS','-O3 -std=c++20']+params+[native_path(f) for f in files]
            with log.open('wb') as f:command(argv,stdout=f,stderr=subprocess.STDOUT)
            built[key]=objects/'Vrv32_observed_soc'
        dest=root/'runs'/row['case'];dest.mkdir(parents=True,exist_ok=True)
        archive=ROOT/'data/unseen/runs'/row['case'];shutil.copyfile(archive/'firmware.hex',dest/'firmware.hex')
        with (dest/'run.log').open('wb') as f:
            command([native_path(built[key]),native_path(dest)]+row['arguments'],cwd=dest,stdout=f,stderr=subprocess.STDOUT)
        actual=json.loads((dest/'observation.json').read_text(encoding='utf-8-sig'))
        expected=json.loads((archive/'observation.json').read_text(encoding='utf-8-sig'))
        for k in ('cycles','retired','untainted_writeback_sha256','pc_instruction_sha256'):
            require(actual['snapshot'][k]==expected['snapshot'][k],'RTL observation differs: '+row['case']+' / '+k)
        require((dest/'uart.log').read_bytes()==(archive/'uart.log').read_bytes(),'UART differs: '+row['case'])
        results.append({'case':row['case'],'status':'passed','cycles':actual['snapshot']['cycles'],
                        'retired':actual['snapshot']['retired'],'uart_sha256':sha(dest/'uart.log')})
        print('PASS: timed native-memory RTL '+row['case'],flush=True)
    (root/'verification.json').write_text(json.dumps({'status':'passed','cases':results},indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()
