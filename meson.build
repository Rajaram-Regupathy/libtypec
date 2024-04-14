project('libtypec','c',
license: 'MIT',
default_options : [
	'warning_level=0'])

conf_data = configuration_data()
conf_data.set('libtypec_VERSION_MAJOR', '0')
conf_data.set('libtypec_VERSION_MINOR', '5')
conf_data.set('libtypec_VERSION_PATCH', '0')


configure_file(input : 'libtypec_config.h.in', output : 'libtypec_config.h', configuration : conf_data)

both_libraries('typec', 'libtypec.c', 'libtypec_sysfs_ops.c', 'libtypec_dbgfs_ops.c',  soversion : '1')