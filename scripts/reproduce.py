"""Recompute the paper's quantitative results from published experiment records."""
from __future__ import annotations
import argparse, csv, gzip, hashlib, json, math
from collections import Counter, defaultdict
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
def load(name):return json.loads((ROOT/name).read_text(encoding='utf-8-sig'))
def rows(name):
    p=ROOT/name
    with (gzip.open(p,'rt',encoding='utf-8-sig',newline='') if p.suffix=='.gz' else p.open(encoding='utf-8-sig',newline='')) as f:
        yield from csv.DictReader(f)
def require(ok,msg):
    if not ok:raise ValueError(msg)
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(4*1024*1024),b''):h.update(b)
    return h.hexdigest()
def verify_manifest():
    manifest=load('FILE_MANIFEST.json')
    for row in manifest:
        p=ROOT/row['path'];require(p.is_file() and sha(p)==row['sha256'],'File integrity: '+row['path'])
    return len(manifest)

def calculate():
    finite=load('data/finite/results.json');pairs=Counter()
    for r in finite['cross_E_owner_age_joins']:
        for p in r['matched_fast_consume_age_pairs']:pairs[p['E0_age'],p['E1_age']]+=p['owners']
    require(sum(pairs.values())==1049602,'Matched snapshot population')
    hist=[Counter(),Counter()]
    for (a,b),n in pairs.items():hist[0][a]+=n;hist[1][b]+=n
    demand=[sum(n for a,n in h.items() if a>1) for h in hist]
    selected=[min(k for k in range(2,44) if sum(n for a,n in h.items() if 1<a<=k)>=.99*demand[e]) for e,h in enumerate(hist)]
    require(demand==[375063,4373] and selected==[18,28],'Demand/window selection')
    require(len(finite['rows'])==152,'Finite experiment count')
    require(all(r['actual_extra_fast_consumes']==r['predicted_extra_fast_consumes'] for r in finite['rows']),'Finite opportunity prediction')
    identity=sum(r['common_retired_prepared_owners'] for r in finite['owner_identity_audit'])
    require(identity==8708288,'Finite/unbounded instruction comparisons')
    for audit in finite['owner_identity_audit']:
        require(all(audit[k]==0 for k in ('operand_writeback_mismatches','missing_predicted_fast_keys','unexpected_fast_keys')),'Instruction comparison')
    original=list(rows('data/admission/workloads.csv'))
    cycles={(r['workload'],int(r['E'])):int(r['T0_cycles']) for r in original}
    names=sorted({r['workload'] for r in original})
    admission_gain=100*(1-math.exp(sum(math.log(cycles[n,1]/cycles[n,0]) for n in names)/len(names)))
    require(round(admission_gain,1)==18.2,'Admission cycle reduction')
    forecast=list(rows('data/forecast/endpoint-metrics.csv'))
    full=[r for r in forecast if r['cohort']=='full' and r['model']=='relative']
    exact=sum(int(r['exact']) for r in full if r['endpoint']=='age')
    denom=sum(int(r['paired_fast']) for r in full if r['endpoint']=='admission')
    mae=sum(int(r['absolute_error_sum']) for r in full if r['endpoint']=='admission')/denom
    require(exact==2177072 and round(mae,3)==.595,'Changed-service prediction')
    unseen=load('data/unseen/results.json')
    require(unseen['source_profiles']+unseen['unbounded_target_cases']+unseen['finite_target_cases']==58,'Unseen count')
    require(all(r['opportunity_error']==0 and r['architectural_identity_matches'] for r in unseen['finite']),'Unseen finite recovery')
    paired=sum(r['paired_fast_owners'] for r in unseen['cases'])
    require(paired==16396,'Unseen paired instructions across both services')
    for case in unseen['cases']:
        if case['paired_fast_owners']:
            require(case['phase_metrics']['repaired_relative']['full']['admission']['MAE']==0,'Unseen endpoint accuracy')
    second=load('data/second-service/results.json')
    require(second['predicted_Khat']==second['actual_K99']==12,'Second service K')
    require(all(r['opportunity_error_actual_minus_prediction']==0 for r in second['finite']),'Second service finite recovery')
    loo=load('data/leave-one-out/results.json')['modes']
    for e in ('E0','E1'):
        require(sum(r['captured'] for r in loo[e]['folds'])/sum(r['demand'] for r in loo[e]['folds'])==loo[e]['heldout_aggregate_capture'],'Leave-one-out pooled capture')
    impl=[]
    for row in load('data/implementation/pacerv.json')['rows']:
        if row['source'] not in ('H3-O','H3-O-R') or row['role']!='V0':continue
        impl.append(dict(name='PaceRV Base' if row['source']=='H3-O' else 'PaceRV Retained',
                         cycles=row['cycles'],LUT=row['LUT'],FF=row['FF'],BRAM=row['DRM'],DSP=row['APM'],
                         setup_ns=row['setup_ns'],evidence='board'))
    vex=load('data/implementation/vex.json');r=vex['physical_resources']
    impl.append(dict(name='Vex EX',cycles=vex['coremark_performance_native_cycles'],LUT=r['physical_LUT'],FF=r['FF'],
                     BRAM=r['DRM'],DSP=r['APM'],setup_ns=vex['physical_setup_wns_ns'],evidence='native RTL'))
    for r in impl:
        r['clock_MHz']=145.125;r['CoreMark_per_MHz']=6_000_000_000/r['cycles']
        r['CoreMark']=r['CoreMark_per_MHz']*145.125
        r['CoreMark_per_MHz_per_kLUT']=1000*r['CoreMark_per_MHz']/r['LUT']
    cycle_errors=dict(
        original_finite_registered=max(r['predicted_saved_cycles_local']-r['actual_saved_cycles'] for r in finite['aggregate'] if r['E']==0),
        second_service=max(r['predicted_saved_cycles']-r['actual_saved_cycles'] for r in second['finite']),
        unseen_matmul=max(r['predicted_saved_cycles']-r['actual_saved_cycles'] for r in unseen['finite']))
    require(list(cycle_errors.values())==[80,2,4],'Elapsed-cycle residuals')
    return dict(matched_snapshot_instructions=sum(pairs.values()),age_pairs=[[*p,n] for p,n in sorted(pairs.items())],
        additional_demand=demand,selected_K99=selected,admission_geomean_cycle_reduction_percent=admission_gain,
        finite_executions=len(finite['rows']),finite_identity_comparisons=identity,
        forecast_paired_lifetimes_exact=exact,forecast_endpoint_MAE=mae,
        unseen_executions=58,unseen_paired_instructions=paired,second_service_K99=12,
        elapsed_cycle_overestimation=cycle_errors,
        leave_one_out={e:dict(meeting=loo[e]['programs_meeting_target'],positive_demand=loo[e]['positive_demand_programs'],
                              pooled_capture=loo[e]['heldout_aggregate_capture']) for e in loo},implementation=impl)

def per_instruction():
    n=0;total=0
    for r in rows('data/forecast/paired-owner-errors.csv.gz'):
        require(int(r['relative_age_error'])==0,'Changed-service lifetime error')
        total+=abs(int(r['relative_admission_error']));n+=1
    require(n==2177072 and round(total/n,3)==.595,'Per-instruction forecast reduction')
    unseen=0
    for case in load('data/unseen/results.json')['cases']:
        stem=f'{case["service"]}/E{case["E"]}'
        predictions={int(r['owner_seq']):r for r in rows('data/unseen/forecasts/'+stem+'/'+case['workload']+'.csv')}
        count=0
        for r in rows('data/unseen/runs/'+stem+'/Kinf/'+case['workload']+'/owner_events.csv.gz'):
            if r['event']!='fast_consume':continue
            pred=predictions[int(r['owner_seq'])]
            require(int(r['admission'])==int(pred['admission_relative']) and int(r['edge'])==int(pred['consume_relative']),'Unseen instruction endpoint')
            count+=1
        require(count==case['paired_fast_owners'],'Unseen paired instruction count');unseen+=count
    require(unseen==16396,'Unseen instruction row count')
    second=Counter()
    for r in rows('data/second-service/owner-errors.csv.gz'):
        second[r['model']]+=1
        if r['model'] in ('stored_relative','repaired_relative'):
            require(int(r['admission_error_actual_minus_prediction'])==int(r['consume_error_actual_minus_prediction'])==int(r['age_error_actual_minus_prediction'])==0,'Second-service error')
    require(second['stored_absolute']==320000,'Second-service ablation row count')
    require(second['repaired_relative']==0,'Relative model has no nonzero error rows')
    return dict(changed_service=n,unseen=unseen,second_service_nonzero_ablation_rows=second['stored_absolute'])

def plots(result,output):
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    plt.rcParams.update({'font.family':'DejaVu Sans','font.size':9,'pdf.fonttype':42,'svg.fonttype':'none'})
    colors=['#D55E00','#009E73'];labels=['Registered','Early']
    fig,axes=plt.subplots(1,2,figsize=(7.2,2.8),layout='constrained')
    for e in (0,1):
        h=Counter()
        for a,b,n in result['age_pairs']:h[(a,b)[e]]+=n
        ks=list(range(1,44));d=result['additional_demand'][e]
        remaining=[sum(n for a,n in h.items() if a>k) for k in ks]
        axes[0].step(ks,[100*(1-n/d) for n in remaining],where='post',color=colors[e],label=labels[e])
        axes[1].step(ks,remaining,where='post',color=colors[e],label=labels[e])
    axes[0].axhline(99,color='#666666',ls=':',lw=.8);axes[0].set_ylabel('Delayed capture (%)')
    axes[1].set_yscale('symlog',linthresh=1);axes[1].set_ylabel('Unserved multiplications')
    for ax in axes:ax.set_xlabel('Validity window K (edges)');ax.grid(alpha=.2);ax.legend(loc='lower right',fontsize=8)
    save(fig,output,'population-capture')
    finite=load('data/finite/results.json')['aggregate']
    fig,axes=plt.subplots(1,2,figsize=(7.2,2.8),layout='constrained')
    for e in (0,1):
        group=[r for r in finite if r['E']==e]
        axes[0].plot([r['K'] for r in group],[r['actual_extra_fast_consumes']/result['additional_demand'][e]*100 for r in group],'-o',color=colors[e],label=labels[e])
        axes[1].plot([r['K'] for r in group],[-r['cycle_prediction_error'] for r in group],'-o',color=colors[e],label=labels[e])
    axes[0].set_ylabel('Measured capture (%)');axes[1].set_ylabel('Predicted minus measured cycles')
    for ax in axes:ax.set_xlabel('Validity window K (edges)');ax.legend();ax.grid(alpha=.2)
    save(fig,output,'finite-prediction')
    fig,axes=plt.subplots(1,3,figsize=(9,3),layout='constrained')
    for ax,key,title in zip(axes,['CoreMark_per_MHz','CoreMark_per_MHz_per_kLUT','LUT'],['CoreMark/MHz','CoreMark/MHz/kLUT','Routed LUTs']):
        vals=[r[key] for r in result['implementation']]
        ax.bar(range(3),vals,color=['#0072B2','#009E73','#777777']);ax.set_xticks(range(3),['Base','Retained','Vex EX']);ax.set_title(title)
    save(fig,output,'implementation')

def save(fig,output,name):
    for ext in ('svg','pdf','png'):fig.savefig(output/(name+'.'+ext),dpi=180)
    import matplotlib.pyplot as plt
    plt.close(fig)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--per-instruction',action='store_true');p.add_argument('--plots',action='store_true');args=p.parse_args()
    count=verify_manifest();result=calculate();output=ROOT/'output';output.mkdir(exist_ok=True)
    result['verified_manifest_files']=count
    if args.per_instruction:result['per_instruction_checks']=per_instruction()
    (output/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    with (output/'implementation.csv').open('w',newline='',encoding='utf-8') as f:
        writer=csv.DictWriter(f,fieldnames=list(result['implementation'][0]));writer.writeheader();writer.writerows(result['implementation'])
    if args.plots:plots(result,output)
    print(json.dumps(result,indent=2));print('PASS: paper result reproduction')

if __name__=='__main__':main()
