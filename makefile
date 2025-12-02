all:
	make clean
	make debug
	make release
debug:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
	@cmake --build build --config Debug
	@rm -rf /Library/Audio/Plug-Ins/VST3/Spectrix.vst3
	@rm -rf /Library/Audio/Plug-Ins/Components/Spectrix.component/
	@cp -r build/source/Spectrix_artefacts/Debug/VST3/Spectrix.vst3 /Library/Audio/Plug-Ins/VST3	
	@cp -r build/source/Spectrix_artefacts/Debug/AU/Spectrix.component /Library/Audio/Plug-Ins/Components

release:
	@cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
	@cmake --build build --config Release
	@rm -rf /Library/Audio/Plug-Ins/VST3/Spectrix.vst3
	@rm -rf /Library/Audio/Plug-Ins/Components/Spectrix.component/
	@cp -r build/source/Spectrix_artefacts/Release/VST3/Spectrix.vst3 /Library/Audio/Plug-Ins/VST3
	@cp -r build/source/Spectrix_artefacts/Release/AU/Spectrix.component /Library/Audio/Plug-Ins/Components

run:
	@./build/source/Spectrix_artefacts/Standalone/Spectrix.app/Contents/MacOS/Spectrix


clean:
	@rm -rf build
	@rm -rf .cache
	@rm -rf /Library/Audio/Plug-Ins/VST3/Spectrix.vst3
	@rm -rf /Library/Audio/Plug-Ins/Components/Spectrix.component/

