"""Pipeline rejection and canonical-byte tests using the exported source artifact."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from starting_memory_ir import IR, ROOT, canonical, fingerprint, load, source_bytes, strict_load, validate

class StartingCatalogTests(unittest.TestCase):
    def setUp(self):
        self.ir=load()

    def checked(self,value):
        return validate(strict_load(canonical(value)),verify_sources=False)

    def test_source_provenance_and_canonical_bytes(self):
        self.assertEqual(canonical(self.ir),IR.read_bytes())
        self.assertEqual(fingerprint(self.ir['entries']),self.ir['semantic_sha256'])

    def test_supported_metadata_only(self):
        for key,value in [('schema_version',2),('schema_version',True),('content_kind','world_memory'),('extractor_version','unknown'),('source_repository_revision','unknown')]:
            with self.subTest(key=key,value=value):
                changed=deepcopy(self.ir); changed[key]=value
                with self.assertRaises(ValueError): self.checked(changed)

    def test_bad_definition_fields(self):
        for key,values in [('grade',[-1,5,0.5,True,'0']),('burn_power',[0,-1,1.5,True,'10',2147483648]),('related_npc',[None,12,[]]),('id',['',12,None]),('title',['',None]),('description',['',None])]:
            for value in values:
                with self.subTest(key=key,value=value):
                    changed=deepcopy(self.ir); changed['entries'][0][key]=value
                    changed['semantic_sha256']=fingerprint(changed['entries'])
                    with self.assertRaises(ValueError): self.checked(changed)

    def test_no_mutable_runtime_fields(self):
        for key in ('burned','residue','faded','erosion','active_loan','guards','anchor_vigil','burn_passives','connections'):
            with self.subTest(key=key):
                changed=deepcopy(self.ir); changed['entries'][0][key]=0
                changed['semantic_sha256']=fingerprint(changed['entries'])
                with self.assertRaises(ValueError): self.checked(changed)

    def test_exact_duplicate_rejected_case_distinct_preserved(self):
        changed=deepcopy(self.ir)
        changed['entries'][0]['id']=changed['entries'][1]['id']
        changed['semantic_sha256']=fingerprint(changed['entries'])
        with self.assertRaises(ValueError): self.checked(changed)
        changed['entries'][0]['id']=changed['entries'][0]['id'].upper()
        changed['semantic_sha256']=fingerprint(changed['entries'])
        self.assertEqual(self.checked(changed)['entries'][0]['id'],changed['entries'][0]['id'])

    def test_order_is_semantic_and_not_sorted(self):
        changed=deepcopy(self.ir); changed['entries'].reverse()
        self.assertNotEqual(fingerprint(changed['entries']),self.ir['semantic_sha256'])
        with self.assertRaises(ValueError): self.checked(changed)
        changed['semantic_sha256']=fingerprint(changed['entries'])
        self.assertEqual(self.checked(changed)['entries'],changed['entries'])

    def test_changed_title_is_detected(self):
        changed=deepcopy(self.ir); changed['entries'][0]['title']+=' temporary'
        with self.assertRaises(ValueError): self.checked(changed)
        changed['semantic_sha256']=fingerprint(changed['entries'])
        self.assertNotEqual(self.checked(changed)['semantic_sha256'],self.ir['semantic_sha256'])

    def test_provenance_hash_or_revision_mismatch(self):
        changed=deepcopy(self.ir); changed['sources'][0]['sha256_utf8_lf']='0'*64
        with self.assertRaisesRegex(ValueError,'Source hash differs'): validate(changed)
        changed=deepcopy(self.ir); changed['source_repository_revision']='0'*40
        with self.assertRaises(subprocess.CalledProcessError): validate(changed)

    def test_unknown_missing_fields_and_sources(self):
        for change in (lambda x:x.update(extra=True),lambda x:x.pop('sources'),lambda x:x['sources'].reverse(),lambda x:x['entries'].clear(),lambda x:x['entries'][0]['localization_ko'].update(extra='oops')):
            changed=deepcopy(self.ir); change(changed)
            with self.assertRaises(ValueError): self.checked(changed)

    def test_duplicate_json_and_noncanonical_bytes(self):
        raw=IR.read_bytes()
        for bad in (raw.replace(b'{',b'{"schema_version":1,',1),b' '+raw,raw.replace(b'\n',b'\r\n'),b'\xef\xbb\xbf'+raw,json.dumps(self.ir,ensure_ascii=True).encode()):
            with self.subTest(bad=bad[:30]):
                with self.assertRaises(ValueError): strict_load(bad)

    def test_utf8_text_and_checkout_normalization(self):
        data='혼합 Text\nNext\n'.encode()
        self.assertEqual(source_bytes(data),source_bytes(data.replace(b'\n',b'\r\n')))
        self.assertIn('빗물'.encode(),IR.read_bytes())
        for value in (b'\xef\xbb\xbf'+data,b'bare\rCR'):
            with self.assertRaises(ValueError): source_bytes(value)
        # Check the real attributes using a temporary index; no staging side effect.
        import os
        with tempfile.TemporaryDirectory() as temp:
            env=os.environ.copy(); env['GIT_INDEX_FILE']=str(Path(temp)/'index')
            subprocess.run(['git','read-tree','HEAD'],cwd=ROOT,env=env,check=True)
            subprocess.run(['git','add','--','.gitattributes',str(IR.relative_to(ROOT))],cwd=ROOT,env=env,check=True,capture_output=True)
            oid=subprocess.check_output(['git','rev-parse',':'+IR.relative_to(ROOT).as_posix()],cwd=ROOT,env=env,text=True).strip()
            for mode in ('true','false'):
                actual=subprocess.check_output(['git','-c','core.autocrlf='+mode,'cat-file','--filters','--path='+IR.relative_to(ROOT).as_posix(),oid],cwd=ROOT,env=env)
                self.assertEqual(hashlib.sha256(actual).hexdigest(),hashlib.sha256(IR.read_bytes()).hexdigest())

if __name__=='__main__': unittest.main()
