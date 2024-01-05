if not libssl_dep.found()
  error('fish plugin requires openssl')
endif

fishlim_sources = [
  'dh1080.cpp',
  'blowfish.cpp',
  'blowfish_cbc.cpp',
  'utils.cpp',
  'ircmessage.cpp',
  'keymanagement.cpp',
  'plugin_hexchat.cpp'
]

shared_module('blowfish', blowfish_sources,
  dependencies: [libgio_dep, hexchat_plugin_dep, libssl_dep],
  install: true,
  install_dir: plugindir,
  name_prefix: '',
  vs_module_defs: 'blowfish.def',
)
