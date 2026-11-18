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

[Files]
Source: "..\build\OctaveSpectrogram_artefacts\Release\VST3\Octave Spectrogram.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\Octave Spectrogram.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
