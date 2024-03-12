# VS Project Renamer

The reason I create this project is simple, I like to copy over previous VS projects and rename them so I don't need to right click on every file and include it in the project or manually import files.

This tool allows me to pass in the new project directory (which is a clone of another project) and enter a new name for it, after I run the tool I can simply open the .sln file and everything will be functional.

## Download

If you'd like to download this tool, please visit the [Releases page](https://github.com/ross-r/vs_project_renamer/releases)

## Usage

```
Usage:
        renamer.exe <project_dir> <new_project_name>
                project_dir = file system path to the folder where the .sln file is
                new_project_name = a string representing the new name for the solution and related project files

Example:
        renamer.exe "C:\Users\Admin\Desktop\Projects\My Visual Studio Project" NewProjectName
```
