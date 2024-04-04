import gzip
import os
import datetime
Import("env")

files = [
  'data_embed/index.html',
  'data_embed/script.js',
  'data_embed/style.css',
  'data_embed/bootstrap.js',
  'data_embed/bootstrap.css',
]

for src in files:
  out = src + ".gz"
  
  
  with open(src, 'rb') as f:
    content = f.read()
    
  if src == 'data_embed/index.html':
    env_vars = env["BOARD"] + "<br>" + ','.join(env["BUILD_FLAGS"]).replace('-Werror -Wall,', '').replace(',-DELEGANTOTA_USE_ASYNC_WEBSERVER=1', '')
    current_date = datetime.datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S') + " UTC"
    build_info = f'{env_vars}<br>Build date: {current_date}'.encode()
    
    content = content.replace(b'%BUILD_INFO%', build_info)
      
  with open(out, 'wb') as f:
    f.write(gzip.compress(content, compresslevel=9))