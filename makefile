all:
	@cmake -B build -G Ninja
	@cmake --build build
	@cp -r build/source/Spectrix_artefacts/VST3/Spectrix.vst3 /Library/Audio/Plug-Ins/VST3
clean:
	@rm -rf build
	@rm -rf .cache
