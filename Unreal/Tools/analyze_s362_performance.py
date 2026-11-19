"""Summarize real S362 captures without treating editor fixtures as game benchmarks."""
from pathlib import Path
import argparse,csv,json,math,statistics

def distribution(values):
    values=sorted(v for v in values if math.isfinite(v))
    if not values: return {'status':'unavailable'}
    percentile=lambda p: values[max(0,math.ceil(p*len(values))-1)]
    return {'status':'measured','samples':len(values),'minimum':values[0],
            'mean':statistics.fmean(values),'median':statistics.median(values),
            'p95_nearest_rank':percentile(.95),'p99_nearest_rank':percentile(.99),'maximum':values[-1]}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('evidence',type=Path);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();e=args.evidence.resolve()
    folder=e/'Artifacts/S362Performance'
    if not folder.is_dir():folder=e/'PerformanceData'
    wall=list(csv.DictReader((folder/'wall_frames.csv').open(encoding='utf-8-sig',newline='')))
    ms=[float(r['wall_ms']) for r in wall]
    result={'scope':'Actual editor presentation fixture: eight authored archetypes, held physical keyboard route, no AI. Includes editor/validation overhead; no Shipping or before/after optimization claim.',
            'source':str(folder),'wall_frame_ms':distribution(ms),'observed_interval_fps':1000/statistics.fmean(ms),
            'player_travel_cm':sum(float(r['player_step_cm']) for r in wall),
            'frames_over_16_67_ms':sum(v>1000/60 for v in ms),'frames_over_33_33_ms':sum(v>1000/30 for v in ms)}
    columns=('FrameTime','GameThreadTime','RenderThreadTime','RHIThreadTime','GPUTime','RHI/DrawCalls',
             'GPUMem/LocalUsedMB','GPUMem/LocalBudgetMB','AnimationParallelEvaluation/TotalTaskTime','Exclusive/GameThread/Animation')
    samples={k:[] for k in columns}
    with (folder/'EnginePresentationEight.csv').open(encoding='utf-8-sig',newline='') as f:
        for row in csv.DictReader(f):
            try:
                if not math.isfinite(float(row.get('FrameTime',''))):continue
            except (ValueError,TypeError):continue
            for k in columns:
                try:
                    v=float(row.get(k,''))
                    if math.isfinite(v) and v>=0:samples[k].append(v)
                except (ValueError,TypeError):pass
    result['engine_csv_counters']={k:distribution(v) for k,v in samples.items()}
    result['engine_csv_limits']='Published engine timing/memory counter names retained verbatim; GPUTime is usable only when positive. Thread/wall/parallel-task scopes differ and are not additive. CSV frames are not assumed aligned with wall-frame rows.'
    inventory=json.loads((folder/'loaded_end.json').read_text(encoding='utf-8-sig'))
    textures=inventory['material_textures']
    result['inventory']={k:inventory[k] for k in ('registered_primitives','visible_primitives','visible_skinned_components','visible_static_components','visible_static_instances','process_loaded_never_stream_textures')}
    result['actual_pixels']=inventory.get('pixels',{'status':'unavailable in this earlier inventory'})
    result['inventory'].update(skeletal_meshes=inventory['skeletal_meshes'],world_material_texture_count=len(textures),
                              world_material_never_stream_count=sum(t['never_stream'] for t in textures),
                              texture_limit='Counts and actual resident mip metadata only; dimensions are not converted to fabricated VRAM byte estimates.')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps({k:result[k] for k in ('wall_frame_ms','observed_interval_fps','player_travel_cm')},ensure_ascii=False))

if __name__=='__main__':main()
