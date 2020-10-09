#!/usr/bin/env python3

import argparse, glob, os, platform, shutil, subprocess, stat, sys, urllib.request

def run_command(command):
	if platform.system() != 'Windows' and os.sep in command[0] and not os.path.isfile(command[0]):
		sys.exit('error: failed to locate {}'.format(os.path.basename(command[0])))

	if subprocess.call(command) != 0:
		sys.exit('error: failed to execute "{}"'.format(' '.join(command)))

def escape_windows_executable_path(executable_path):
	if ' ' not in executable_path:
		return executable_path

	segments = executable_path.split('\\')
	escaped_segments = []

	for segment in segments:
		if ' ' in segment:
			segment = '"{}"'.format(segment)

		escaped_segments.append(segment)

	return '\\'.join(escaped_segments)

def make_path(root, directories):
	if not os.path.exists(root):
		os.mkdir(root)

	for directory in directories:
		root = os.path.join(root, directory)

		if not os.path.exists(root):
			os.mkdir(root)

def get_executable(name, url=None, tools_path=None, is_optional=False):
	exectable_path = None

	if tools_path != None:
		exectable_path = os.path.join(tools_path, name)

		if os.path.isfile(exectable_path):
			return exectable_path

	for directory in os.getenv('PATH', '').split(os.pathsep):
		possible_exectable_path = os.path.join(directory, name)

		if os.path.isfile(possible_exectable_path) and os.access(possible_exectable_path, os.X_OK):
			return possible_exectable_path

	if tools_path != None and url != None:
		print('info: downloading {} from {}'.format(name, url))

		urllib.request.urlretrieve(url, exectable_path)
		os.chmod(exectable_path, os.stat(exectable_path).st_mode | stat.S_IEXEC)

	if not is_optional and exectable_path == None:
		sys.exit('error: failed to locate {}'.format(name))

	return exectable_path

def deploy_locale(source_path, target_path):
	target_locale_path = os.path.join(target_path, 'locale')

	os.mkdir(target_locale_path)

	for file in glob.glob(os.path.join(source_path, 'resources', 'translations', '*.qm')):
		shutil.copy(file, target_locale_path)

def deploy_linux(arguments):
	tools_path = None

	if not arguments.disable_tools_download:
		tools_path = os.path.join(os.path.abspath(os.path.dirname(sys.argv[0])), 'appimage-tools')

		if not os.path.isdir(tools_path):
			os.mkdir(tools_path)

	release_name = 'otter-browser' + ('' if (arguments.release_name == '') else '-' + arguments.release_name)
	package_formats = ['appimage']

	if arguments.package_formats != 'default':
		package_formats = arguments.package_formats.split(',')

	appdir_deploy_command = get_executable('linuxdeploy-x86_64.AppImage', 'https://bintray.com/qtproject/linuxdeploy-mirror/download_file?file_path=2020-06-03%2Flinuxdeploy-x86_64.AppImage', tools_path)

	get_executable('linuxdeploy-plugin-qt-x86_64.AppImage', 'https://bintray.com/qtproject/linuxdeploy-mirror/download_file?file_path=2020-06-03%2Flinuxdeploy-plugin-qt-x86_64.AppImage', tools_path)

	appimage_path = os.path.join(arguments.target_path, 'otter-browser')

	make_path(appimage_path, ['usr', 'share', 'applications'])
	make_path(appimage_path, ['usr', 'share', 'icons', 'hicolor'])
	make_path(appimage_path, ['usr', 'share', 'otter-browser'])
	shutil.copy(os.path.join(arguments.source_path, 'otter-browser.desktop'), os.path.join(appimage_path, 'usr/share/applications/otter-browser.desktop'))

	icons_path = os.path.join(appimage_path, 'usr/share/icons/hicolor')
	icons = [16, 32, 48, 64, 128, 256, 'scalable']

	for size in icons:
		icon_directory = 'scalable'
		source_name = 'otter-browser.svg'
		target_name = 'otter-browser.svg'

		if isinstance(size, int):
			icon_directory = '{}x{}'.format(size, size)
			source_name = 'otter-browser-{}.png'.format(size)
			target_name = 'otter-browser.png'

		make_path(icons_path, [icon_directory, 'apps'])
		shutil.copy(os.path.join(arguments.source_path, 'resources/icons', source_name), os.path.join(icons_path, icon_directory, 'apps', target_name))

	deploy_locale(arguments.source_path, os.path.join(appimage_path, 'usr/share/otter-browser'))
	os.putenv('LD_LIBRARY_PATH', '{}:{}'.format(os.path.join(arguments.qt_path, 'lib'), os.getenv('LD_LIBRARY_PATH')))
	os.putenv('QMAKE', os.path.join(arguments.qt_path, 'bin/qmake'))
	run_command([appdir_deploy_command, '--plugin=qt', '--executable={}'.format(os.path.join(arguments.build_path, 'otter-browser')), '--appdir={}'.format(appimage_path)])
	shutil.rmtree(os.path.join(appimage_path, 'usr/share/doc/'), ignore_errors=True)

	libs_path = os.path.join(appimage_path, 'usr/lib')
	redundant_libs = ['libgst*-1.0.*', 'libFLAC.*', 'libogg.*', 'libvorbis*.*', 'libmount.*', 'libpulse*.*', 'libsystemd.*', 'libxml2.*']

	for pattern in redundant_libs:
		for file in glob.glob(os.path.join(libs_path, pattern)):
			os.unlink(file)

	for file in glob.glob(os.path.join(libs_path, 'libicu*.*')):
		if not os.path.exists(os.path.join(arguments.qt_path, 'lib', os.path.basename(file))):
			os.unlink(file)

	if 'appimage' in package_formats:
		appimage_tool_command = get_executable('appimagetool-x86_64.AppImage', 'https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage', tools_path)

		run_command([appimage_tool_command, appimage_path, os.path.join(arguments.target_path, release_name + '-x86_64.AppImage')])

	if 'zip' in package_formats:
		run_command(['zip', '-r', release_name + '.zip', appimage_path])

	if not arguments.preserve_deployment_directory:
		shutil.rmtree(appimage_path)

def deploy_macos(arguments):
	macdeployqt_command = os.path.join(arguments.qt_path, 'bin/macdeployqt')

	if not os.path.isfile(macdeployqt_command):
		sys.exit('error: failed to locate "{}"'.format(macdeployqt_command))

	package_formats = ['zip']

	if arguments.package_formats != 'default':
		package_formats = arguments.package_formats.split(',')

	app_path = os.path.join(arguments.target_path, 'OtterBrowser.app')

	shutil.copytree(os.path.join(arguments.build_path, 'Otter Browser.app'), app_path)
	run_command([macdeployqt_command, app_path])

	if 'zip' in package_formats:
		run_command(['zip', '-r', release_name + '.zip', app_path])

	if not arguments.preserve_deployment_directory:
		shutil.rmtree(app_path)

def deploy_windows(arguments):
	windeployqt_command = os.path.join(arguments.qt_path, r'bin\windeployqt.exe')
	seven_z_command = r'C:\Program Files\7-Zip\7z.exe'
	inno_setup_command = r'C:\Program Files (x86)\Inno Setup 6\ISCC.exe'

	if not os.path.isfile(windeployqt_command):
		sys.exit('error: failed to locate "{}"'.format(windeployqt_command))

	is_64bit = '_64' in arguments.qt_path[-5:]
	release_name = 'otter-browser-win' + ('64' if is_64bit else '32') + ('' if (arguments.release_name == '') else '-' + arguments.release_name)
	package_formats = ['iss', 'zip', '7z']

	if arguments.package_formats != 'default':
		package_formats = arguments.package_formats.split(',')

	target_installer_path = os.path.join(arguments.target_path, 'input')

	os.mkdir(target_installer_path)
	shutil.copy(os.path.join(arguments.build_path, 'otter-browser.exe'), target_installer_path)
	shutil.copy(os.path.join(arguments.source_path, 'COPYING'), target_installer_path)
	deploy_locale(arguments.source_path, os.path.join(arguments.target_path, 'input'))
	run_command([escape_windows_executable_path(windeployqt_command), os.path.join(arguments.target_path, r'input\otter-browser.exe')])

	dlls_path = os.path.join(arguments.qt_path, 'bin')
	extra_dlls = ['libxml2*.dll', 'libxslt*.dll']

	for pattern in extra_dlls:
		for file in glob.glob(os.path.join(dlls_path, pattern)):
			shutil.copy(file, target_installer_path)

	for pattern in arguments.extra_libs:
		matches = glob.glob(pattern)

		if len(matches) == 0:
			sys.exit('error: failed to locate {}'.format(pattern))

		for file in matches:
			shutil.copy(file, target_installer_path)

	redundant_plugins = ['playlistformats', 'position', 'qmltooling', 'scenegraph', 'sensorgestures', 'sensors']

	for directory in redundant_plugins:
		shutil.rmtree(os.path.join(target_installer_path, directory), ignore_errors=True)

	if 'iss' in package_formats:
		if not os.path.isfile(inno_setup_command):
			inno_setup_command = get_executable('ISCC.exe')

		inno_setup_arguments = '/DOtterWorkingDir="{}"'.format(arguments.target_path)

		if is_64bit:
			inno_setup_arguments += ' /DOtterWin64=1'

		os.system('{} {} "{}"'.format(escape_windows_executable_path(inno_setup_command), inno_setup_arguments, os.path.join(arguments.source_path, r'packaging\otter-browser.iss')))

	target_release_path = os.path.join(arguments.target_path, release_name)

	os.rename(target_installer_path, target_release_path)

	if arguments.enable_portable:
		with open(os.path.join(target_release_path, 'arguments.txt'), 'w') as file:
			file.write('--portable')

	if '7z' in package_formats:
		if not os.path.isfile(seven_z_command):
			seven_z_command = get_executable('7z.exe', is_optional=True)

		if seven_z_command != None:
			run_command([seven_z_command, 'a', '{}.7z'.format(target_release_path), target_release_path])

	if 'zip' in package_formats:
		run_command(['powershell', 'Compress-Archive', '"{}"'.format(target_release_path), '"{}.zip"'.format(target_release_path)])

	if not arguments.preserve_deployment_directory:
		shutil.rmtree(target_release_path)

	debug_files_path = arguments.debug_path

	if debug_files_path == '':
		debug_files_path = arguments.target_path

	debug_file_path = os.path.join(arguments.build_path, 'otter-browser.pdb')

	if os.path.isfile(debug_file_path):
		shutil.copy(debug_file_path, os.path.join(debug_files_path, release_name + '.pdb'))

if __name__ == '__main__':
	parser = argparse.ArgumentParser(description='Otter Browser deployment tool.')
	parser.add_argument('--build-path', help='Path to the build directory', default=os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '../build')))
	parser.add_argument('--source-path', help='Path to the source directory', default=os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '..')))
	parser.add_argument('--target-path', help='Path to the output directory', default=os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), '../output')))
	parser.add_argument('--debug-path', help='Path to the debug files directory', default='')
	parser.add_argument('--qt-path', help='Path to the Qt directory', default=os.getenv('QTDIR', ''))
	parser.add_argument('--release-name', help='Release name', default='')
	parser.add_argument('--package-formats', help='Comma separated list of package formats to create', default='default')
	parser.add_argument('--extra-libs', help='Paths to the extra libraries to include', default=[], nargs='*')
	parser.add_argument('--enable-portable', help='Force portable mode', action='store_true')
	parser.add_argument('--disable-tools-download', help='Disable download of missing deployment tools', action='store_true')
	parser.add_argument('--preserve-deployment-directory', help='Preserve deployment directory after creating packages', action='store_true')

	arguments = parser.parse_args()

	if arguments.qt_path == '':
		sys.exit('error: the following arguments are required: --qt-path')

	if platform.system() == 'Linux':
		deploy_linux(arguments)
	elif platform.system() == 'Darwin':
		deploy_macos(arguments)
	elif platform.system() == 'Windows':
		deploy_windows(arguments)
