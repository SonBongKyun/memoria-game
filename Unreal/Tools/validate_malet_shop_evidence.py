"""Phase 1O invokes every retained N full-state check and exact new shop comparisons."""
import argparse
from pathlib import Path
from validate_malet_firebomb_evidence import validate
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--automation-dir',type=Path,required=True);p.add_argument('--evidence-dir',type=Path,required=True)
    raise SystemExit(validate(p.parse_args(),shop_frontier=True))
