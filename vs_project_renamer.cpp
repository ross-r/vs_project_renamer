#include <windows.h>

#include <cstdio>
#include <string>
#include <filesystem>
#include <initializer_list>
#include <unordered_map>
#include <fstream>
#include <ostream>
#include <regex>
#include <format>

// rename.exe project_dir new_project_name

// VS2022 has the following project files which determine the project/solution name and scopes.
//    *.sln
//    *.filters
//    *.user
//    *.vcxproj
//    *.vcxproj.user
//
// There's also a .vs folder that has some metadata but it doesn't appear that needs any editing done to it.

// There is a GUID for each project, maybe we should generate a new GUID when we're replacing old project names?

#define FILE_SLN              TEXT( ".sln" )
#define FILE_FILTERS          TEXT( ".filters" )
#define FILE_USER             TEXT( ".user" )
#define FILE_VCXPROJ          TEXT( ".vcxproj" )
#define FILE_VCXPROJ_USER     TEXT( ".vcxproj.user" )

using file_map_t = std::unordered_map< std::wstring, std::wstring >;
using wstr_init_list_t = std::initializer_list< std::wstring >;

//
// Given a list of extensions and an extension, only return true when the extension is present in the supplied list.
//
const bool filter_extension( const wstr_init_list_t filter, const std::wstring& extension ) {
  for( auto it = filter.begin(); it != filter.end(); ++it ) {
    if( ( *it ) == extension ) {
      return true;
    }
  }

  return false;
}

//       Issue: the .extension() function for std::filesystem likely does a std::string::find_last_of('.') to find the beginning of an extension
//              the problem with doing this is that it will always return the last index where a . occurs, evidently, that will skip over
//              a portion of the file extension described below, a solution would be validate the path is a file and iterate the file name
//              from the end to the beginning and storing the lowest index of a '.' character and treating end-low as the extension buffer.
std::filesystem::path get_extension( const std::filesystem::path& p ) {
  if( !p.has_filename() ) {
    return p;
  }

  const auto& filename = p.filename();
  const auto pos = filename.wstring().find_first_of( L'.' );
  return filename.wstring().substr( pos, filename.wstring().length() );
}

//
// This routine takes in a root directory and iterates only the files in that directory, filters by extension and then returns an unordered map
// where the key value is the file extension.
// 
// This routine is expected to only collect 1 file path per extension and doesn't support multiple paths for a single extension.
//
const file_map_t get_files( const std::wstring_view& directory, const wstr_init_list_t filter ) {
  file_map_t out{};

  for( const auto& entry : std::filesystem::directory_iterator( directory ) ) {
    const auto& path = entry.path();
    if( !path.has_extension() ) {
      continue;
    }

    const auto& extension = get_extension( path );

    // We only care about these two file types.
    if( !filter_extension( filter, extension ) ) {
      continue;
    }

    out.emplace( extension, path.c_str() );
  }

  return out;
}

const std::wstring read_file_contents( const std::wstring_view& path ) {
  // Open the file as input and consume file buffer.
  std::wfstream fs( path.data(), std::ios::in | std::ios::ate );
  if( !fs.is_open() ) {
    return {};
  }

  // Since the "ate" flag seeks to the end of the file immediately, .tellg will return the file size.
  const size_t file_size = fs.tellg();

  // Initialize an empty string filled with null terms for the given size of bytes.
  std::wstring buffer( file_size, '\0' );

  // Reset the seek position and read the entire file stream buffer.
  fs.seekg( 0 );
  fs.read( &buffer[ 0 ], file_size );

  // Close the file and return the buffer.
  fs.close();

  return buffer;
}

const void write_contents_to_file( const std::filesystem::path& path, const std::wstring_view& contents ) {
  std::wofstream fs( path, std::ios::out );
  if( !fs.is_open() ) {
    return;
  }

  fs.write( contents.data(), contents.size() );
  fs.close();
}

void update_solution( const std::filesystem::path& path, const std::wstring_view& new_name ) {
  printf( "Updating solution file..\n" );

  std::wstring filename( path.filename() );
  std::wstring extension( path.extension() );

  // Remove the extension from the file name.
  filename = filename.substr( 0, filename.length() - extension.length() );

  // Read the entirety of the file contents into a buffer.
  auto contents = read_file_contents( path.c_str() );

  // Okay, so we want to construct 2 regex patterns, one that matches "[filename]" and "[filename].vcxproj"
  std::wregex pattern1( std::format( TEXT( "(\"{0}\")" ), filename ) );
  std::wregex pattern2( std::format( TEXT( "(\"{0}.vcxproj\")" ), filename ) );

  // Replace the regex patterns in the file contents buffer.
  contents = std::regex_replace( contents, pattern1, std::format( TEXT( "\"{0}\"" ), new_name ) );
  contents = std::regex_replace( contents, pattern2, std::format( TEXT( "\"{0}{1}\"" ), new_name, FILE_VCXPROJ ) );

  // Create the new file path
  const std::wstring new_filename = std::format( TEXT( "{0}{1}" ), new_name, FILE_SLN );
  const auto new_path = std::filesystem::current_path() / new_filename;

  // Rename the old solution file and append the ".old" extension in the event anything goes wrong.
  std::filesystem::rename( path, std::format( TEXT( "{0}{1}.old" ), filename, extension ) );

  // Now finally write the new solution contents to the new file path.
  write_contents_to_file( new_path, contents );
}

void update_vcxproj( const std::filesystem::path& path, const std::wstring_view& new_name ) {
  printf( "Updating vcxproj file..\n" );

  std::wstring filename( path.filename() );
  std::wstring extension( path.extension() );

  // Remove the extension from the file name.
  filename = filename.substr( 0, filename.length() - extension.length() );

  // Read the entirety of the file contents into a buffer.
  auto contents = read_file_contents( path.c_str() );

  //  <RootNamespace>(.*?)<\/RootNamespace>
  std::wregex pattern( TEXT( "<RootNamespace>(.*?)<\/RootNamespace>" ) );

  const auto& replacement = std::format( TEXT( "<RootNamespace>{0}</RootNamespace>" ), new_name );
  contents = std::regex_replace( contents, pattern, replacement );

  // Create the new file path
  const std::wstring new_filename = std::format( TEXT( "{0}{1}" ), new_name, FILE_VCXPROJ );
  const auto new_path = std::filesystem::current_path() / new_filename;

  // Rename the old solution file and append the ".old" extension in the event anything goes wrong.
  std::filesystem::rename( path, std::format( TEXT( "{0}{1}.old" ), filename, extension ) );

  // Now finally write the new solution contents to the new file path.
  write_contents_to_file( new_path, contents );
}

void print_help() {
  printf( "\n" );
  printf( "Usage:\n" );
  printf( "\trenamer.exe <project_dir> <new_project_name>\n" );
  printf( "\t\tproject_dir = file system path to the folder where the .sln file is\n" );
  printf( "\t\tnew_project_name = a string representing the new name for the solution and related project files\n" );
  printf( "\n" );
  printf( "Example:\n" );
  printf( "\trenamer.exe \"C:\\Users\\Admin\\Desktop\\Projects\\My Visual Studio Project\" NewProjectName\n" );
  printf( "\n" );
  printf( "Created by ross-r (https://github.com/ross-r)\n" );
  printf( "\n" );
}

int wmain( int argc, wchar_t* argv[] ) {
  if( argc != 3 ) {
    print_help();
    return 1;
  }

  const std::wstring project_dir = argv[ 1 ];
  const std::wstring new_project_name = argv[ 2 ];

  if( !std::filesystem::is_directory( project_dir ) ) {
    printf( "ERROR: Invalid project directory.\n" );
    print_help();
    return 1;
  }

  // Update the current working directory to be the project path.
  std::filesystem::current_path( project_dir );
  
  // Get all files in the root project directory and select the files we need based on their extension.
  const file_map_t files = get_files( project_dir, { FILE_SLN, FILE_VCXPROJ } );

  // .sln
  //    Project("{GUID}") = "NAME", "NAME.vcxproj", "{GUID}"
  const auto& sln_path = files.at( FILE_SLN );
  update_solution( sln_path, new_project_name );

  // .vcxproj
  //    XML file format which needs its RootNamespace property updated
  const auto& vcxproj_path = files.at( FILE_VCXPROJ );
  update_vcxproj( vcxproj_path, new_project_name );

  // ... and we also want to rename all the remaining files to the new project name.
  printf( "Renaming old project files.." );
  for( const auto& [extension, path ] : get_files( project_dir, { FILE_FILTERS, FILE_USER, FILE_VCXPROJ_USER } ) ) {
    const auto& new_path = std::format( TEXT( "{0}{1}" ), new_project_name, extension );
    std::filesystem::rename( path, new_path );
  }

  printf( "Done!" );

  return 0;
}