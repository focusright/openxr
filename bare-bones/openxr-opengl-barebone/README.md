openxr_loader.lib
$(SolutionDir)include
$(SolutionDir)lib
xcopy /y /d  "$(SolutionDir)lib\*.dll" "$(OutDir)"