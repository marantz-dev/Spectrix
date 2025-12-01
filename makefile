all:
	@cmake -B build -G Ninja
	@cmake --build build --config Debug
	@rm -rf /Library/Audio/Plug-Ins/VST3/Spectrix.vst3
	@rm -rf /Library/Audio/Plug-Ins/Components/Spectrix.component/
	@cp -r build/source/Spectrix_artefacts/VST3/Spectrix.vst3 /Library/Audio/Plug-Ins/VST3
	@cp -r build/source/Spectrix_artefacts/AU/Spectrix.component /Library/Audio/Plug-Ins/Components

release:
	@cmake -B build -G Ninja 
	@cmake --build build --config Release
	@rm -rf /Library/Audio/Plug-Ins/VST3/Spectrix.vst3
	@rm -rf /Library/Audio/Plug-Ins/Components/Spectrix.component/
	@cp -r build/source/Spectrix_artefacts/VST3/Spectrix.vst3 /Library/Audio/Plug-Ins/VST3
	@cp -r build/source/Spectrix_artefacts/AU/Spectrix.component /Library/Audio/Plug-Ins/Components

clean:
	@rm -rf build
	@rm -rf .cache
