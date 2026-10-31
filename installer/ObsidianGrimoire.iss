; Inno Setup script - builds TRANCE-6-Obsidian-Grimoire-Setup.exe
; Installs all eight plugins of The TRANCE-6 Obsidian Grimoire as one set.
; Requires Release builds first:  cmake -S .. -B ..\build  &&  cmake --build ..\build --config Release

#define Suite "The TRANCE-6 Obsidian Grimoire"
#define Publisher "Red Flame Entertainment"
#define Ver "1.0.0"

[Setup]
AppName={#Suite}
AppVersion={#Ver}
AppPublisher={#Publisher}
DefaultDirName={autopf}\{#Suite}
DefaultGroupName={#Suite}
OutputBaseFilename=TRANCE-6-Obsidian-Grimoire-Setup
OutputDir=Output
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
DisableProgramGroupPage=yes
WizardStyle=modern

[Types]
Name: "full";   Description: "All eight plugins"
Name: "custom"; Description: "Choose plugins"; Flags: iscustom

[Components]
Name: "oracle";     Description: "ORACLE - hybrid wavetable synth";       Types: full
Name: "sigil";      Description: "SIGIL - modal resonator synth";         Types: full
Name: "ethereal";   Description: "ETHEREAL - galactic space reverb";      Types: full
Name: "incant";     Description: "INCANT - groove generator";             Types: full
Name: "aether";     Description: "AETHER - sample-stack synth";           Types: full
Name: "torment";    Description: "TORMENT - hi-fi lo-fi processor";       Types: full
Name: "foundation"; Description: "FOUNDATION - saturation + loudness";    Types: full
Name: "cleanse";    Description: "CLEANSE - mastering / dynamic EQ";      Types: full

[Files]
; Each plugin: VST3 to the shared VST3 folder every DAW scans, plus a stand-alone app.
Source: "..\build\ORACLE_artefacts\Release\VST3\ORACLE.vst3\*";          DestDir: "{commoncf64}\VST3\ORACLE.vst3";     Components: oracle;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\ORACLE_artefacts\Release\Standalone\ORACLE.exe";       DestDir: "{app}\ORACLE";                      Components: oracle;     Flags: ignoreversion
Source: "..\build\SIGIL_artefacts\Release\VST3\SIGIL.vst3\*";            DestDir: "{commoncf64}\VST3\SIGIL.vst3";      Components: sigil;      Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\SIGIL_artefacts\Release\Standalone\SIGIL.exe";         DestDir: "{app}\SIGIL";                       Components: sigil;      Flags: ignoreversion
Source: "..\build\ETHEREAL_artefacts\Release\VST3\ETHEREAL.vst3\*";      DestDir: "{commoncf64}\VST3\ETHEREAL.vst3";   Components: ethereal;   Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\ETHEREAL_artefacts\Release\Standalone\ETHEREAL.exe";   DestDir: "{app}\ETHEREAL";                    Components: ethereal;   Flags: ignoreversion
Source: "..\build\INCANT_artefacts\Release\VST3\INCANT.vst3\*";          DestDir: "{commoncf64}\VST3\INCANT.vst3";     Components: incant;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\INCANT_artefacts\Release\Standalone\INCANT.exe";       DestDir: "{app}\INCANT";                      Components: incant;     Flags: ignoreversion
Source: "..\build\AETHER_artefacts\Release\VST3\AETHER.vst3\*";          DestDir: "{commoncf64}\VST3\AETHER.vst3";     Components: aether;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\AETHER_artefacts\Release\Standalone\AETHER.exe";       DestDir: "{app}\AETHER";                      Components: aether;     Flags: ignoreversion
Source: "..\build\TORMENT_artefacts\Release\VST3\TORMENT.vst3\*";        DestDir: "{commoncf64}\VST3\TORMENT.vst3";    Components: torment;    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\TORMENT_artefacts\Release\Standalone\TORMENT.exe";     DestDir: "{app}\TORMENT";                     Components: torment;    Flags: ignoreversion
Source: "..\build\FOUNDATION_artefacts\Release\VST3\FOUNDATION.vst3\*";  DestDir: "{commoncf64}\VST3\FOUNDATION.vst3"; Components: foundation; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\FOUNDATION_artefacts\Release\Standalone\FOUNDATION.exe";DestDir: "{app}\FOUNDATION";                 Components: foundation; Flags: ignoreversion
Source: "..\build\CLEANSE_artefacts\Release\VST3\CLEANSE.vst3\*";        DestDir: "{commoncf64}\VST3\CLEANSE.vst3";    Components: cleanse;    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\build\CLEANSE_artefacts\Release\Standalone\CLEANSE.exe";     DestDir: "{app}\CLEANSE";                     Components: cleanse;    Flags: ignoreversion

[Icons]
Name: "{autoprograms}\ORACLE";     Filename: "{app}\ORACLE\ORACLE.exe";         Components: oracle
Name: "{autoprograms}\SIGIL";      Filename: "{app}\SIGIL\SIGIL.exe";           Components: sigil
Name: "{autoprograms}\ETHEREAL";   Filename: "{app}\ETHEREAL\ETHEREAL.exe";     Components: ethereal
Name: "{autoprograms}\INCANT";     Filename: "{app}\INCANT\INCANT.exe";         Components: incant
Name: "{autoprograms}\AETHER";     Filename: "{app}\AETHER\AETHER.exe";         Components: aether
Name: "{autoprograms}\TORMENT";    Filename: "{app}\TORMENT\TORMENT.exe";       Components: torment
Name: "{autoprograms}\FOUNDATION"; Filename: "{app}\FOUNDATION\FOUNDATION.exe"; Components: foundation
Name: "{autoprograms}\CLEANSE";    Filename: "{app}\CLEANSE\CLEANSE.exe";       Components: cleanse
