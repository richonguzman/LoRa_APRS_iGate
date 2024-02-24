import gzip

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
      
  with open(out, 'wb') as f:
    f.write(gzip.compress(content, compresslevel=9))