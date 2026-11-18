[Setup]
AppName=Octave Spectrogram
AppVersion=1.0.0
AppPublisher=OctaveTools
DefaultDirName={autopf}\Octave Spectrogram
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=OctaveSpectrogram-Setup
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma
SolidCompression=yes
WizardStyle=modern
FinishedLabel=Octave Spectrogram is installed.%n%nOpen FL Studio, go to Options > Manage plugins and click Find plugins. Then add it to a mixer insert.

[Files]
Source: "..\build\OctaveSpectrogram_artefacts\Release\VST3\Octave Spectrogram.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\Octave Spectrogram.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
