============================================================
Sylredhel - Slippery Fairy v2.0
Installation Guide (Windows / macOS)
============================================================


[Windows 설치 방법]
------------------------------------------------------------
1. 압축을 풉니다.
2. "Slippery Fairy.vst3" 폴더 전체를 아래 경로에 복사합니다.

   C:\Program Files\Common Files\VST3\

3. DAW를 재실행하거나 플러그인을 다시 스캔합니다.
   (제조사: Sylredhel -> Slippery Fairy)


[macOS 설치 방법 (Apple Silicon & Intel 공용)]
------------------------------------------------------------
1. 압축을 풉니다.
2. 사용하는 DAW에 맞춰 파일을 아래 경로에 복사합니다.

   - VST3 (Ableton Live, Cubase, Studio One, Reaper 등):
     "Slippery Fairy.vst3" 파일을 아래 폴더에 복사
     /Library/Audio/Plug-Ins/VST3/

   - AU (Logic Pro, GarageBand, Ableton Live 등):
     "Slippery Fairy.component" 파일을 아래 폴더에 복사
     /Library/Audio/Plug-Ins/Components/

   * Finder에서 이동하려면: 상단 메뉴 [이동] -> [폴더로 이동] (Cmd + Shift + G)

3. (필수) macOS 보안 경고(Gatekeeper) 해제
   Apple 유료 공증 없이 배포되는 무료 플러그인이므로,
   터미널(Terminal)을 열고 아래 명령어를 그대로 복사·붙여넣기 후 Enter를 누르세요.
   (Mac 로그인 비밀번호 입력 시 화면에 글자가 보이지 않아도 입력 후 Enter)

   [시스템 폴더 /Library/... 에 설치한 경우 - VST3]
   sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Slippery Fairy.vst3"

   [시스템 폴더 /Library/... 에 설치한 경우 - AU]
   sudo xattr -cr "/Library/Audio/Plug-Ins/Components/Slippery Fairy.component"

   [개인 폴더 ~/Library/... 에 설치한 경우 - VST3 / AU]
   xattr -cr "$HOME/Library/Audio/Plug-Ins/VST3/Slippery Fairy.vst3"
   xattr -cr "$HOME/Library/Audio/Plug-Ins/Components/Slippery Fairy.component"

4. DAW를 재실행하고 플러그인을 다시 스캔합니다.
   * Logic Pro에서 AU가 바로 보이지 않으면 터미널에 아래 명령어를 실행하거나 Mac을 재시동하세요:
     killall -9 AudioComponentRegistrar


============================================================
[Windows Installation (EN)]
------------------------------------------------------------
1. Extract this archive.
2. Copy the "Slippery Fairy.vst3" folder into:
   C:\Program Files\Common Files\VST3\
3. Rescan plugins in your DAW (Manufacturer: Sylredhel).


[macOS Installation (EN - Universal arm64 / x86_64)]
------------------------------------------------------------
1. Extract this archive.
2. Copy the plugin bundle(s) to:
   - VST3: /Library/Audio/Plug-Ins/VST3/Slippery Fairy.vst3
   - AU:   /Library/Audio/Plug-Ins/Components/Slippery Fairy.component
3. Remove macOS quarantine flag in Terminal (copy & paste):

   sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Slippery Fairy.vst3"
   sudo xattr -cr "/Library/Audio/Plug-Ins/Components/Slippery Fairy.component"

   (If installed under your user Library ~/Library/... instead:)
   xattr -cr "$HOME/Library/Audio/Plug-Ins/VST3/Slippery Fairy.vst3"
   xattr -cr "$HOME/Library/Audio/Plug-Ins/Components/Slippery Fairy.component"

4. Restart your DAW and rescan plugins.
   (For Logic Pro AU cache refresh, run: killall -9 AudioComponentRegistrar)
