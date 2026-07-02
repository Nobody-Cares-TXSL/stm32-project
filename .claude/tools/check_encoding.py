import os
import sys

errors = 0
for root, dirs, files in os.walk('code'):
    for f in files:
        if f.endswith('.c') or f.endswith('.h'):
            path = os.path.join(root, f)
            try:
                with open(path, 'rb') as fp:
                    content = fp.read()
                content.decode('gb2312')
            except UnicodeDecodeError as e:
                print(f'ENCODING ERROR: {path} - {e}')
                errors += 1
            except Exception as e:
                print(f'READ ERROR: {path} - {e}')
                errors += 1

if errors == 0:
    print('All source files decode as GB2312 OK')
else:
    print(f'Total encoding errors: {errors}')
    sys.exit(1)