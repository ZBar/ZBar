import sys, os
from plistlib import readPlist

# General configuration

extensions = []
templates_path = ['ext']
source_suffix = '.rst'
master_doc = 'index'
exclude_patterns = ['.#*']

project = u'ZBar iPhone SDK'
copyright = u'2010-2012, Jeff Brown et al'

today_fmt = '%Y-%m-%d'
info = readPlist('../res/ZBarSDK-Info.plist')
version = 'X.Y'
if info:
    version = info['CFBundleVersion']
release = version

#add_module_names = False

pygments_style = 'sphinx'
highlight_language = 'objc'
primary_domain = 'cpp'

# Options for HTML output

html_theme = 'default'
html_theme_options = {
    'bgcolor': 'white',
    'textcolor': 'black',
    'linkcolor': '#247',
    'headbgcolor': '#edeff0',
    'headtextcolor': '#247',
    'headlinkcolor': '#c11',
    'sidebarbgcolor': '#247',
    'sidebartextcolor': 'white',
    'sidebarlinkcolor': '#cde',
    'relbarbgcolor': '#247',
    'relbartextcolor': '#ccc',
    'relbarlinkcolor': 'white',
    'footerbgcolor': 'white',
    'footertextcolor': 'black',
    'codebgcolor': '#dfe',
    'codetextcolor': 'black',
}

html_short_title = 'ZBarSDK ' + version
html_title = 'ZBar iPhone SDK Documentation'
html_static_path = ['static']
html_favicon = '../../zbar.ico'
html_style = 'style.css'
html_use_modindex = False
html_use_index = False
html_copy_source = False
html_show_sourcelink = False
htmlhelp_basename = 'doc'

# Options for LaTeX output

latex_paper_size = 'letter'
latex_font_size = '10pt'

# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title, author, documentclass [howto/manual])
latex_documents = [
  ('index', 'ZBarSDK.tex', u'ZBar iPhone SDK Documentation',
   u'Jeff Brown', 'manual'),
]

#latex_logo = ''
#latex_use_parts = False
#latex_preamble = ''
#latex_appendices = []
#latex_use_modindex = False
