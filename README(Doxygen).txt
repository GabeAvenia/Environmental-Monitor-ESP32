# Doxygen Documentation for Environmental Monitor Project

This README explains how to use Doxygen to generate code documentation for the Environmental Monitor project.

## Overview

Doxygen is a documentation generator tool that extracts comments from source code and creates comprehensive 
HTML documentation. For this Environmental Monitor project, Doxygen will generate detailed class hierarchy 
diagrams, function references, and other useful documentation to help understand the codebase.

## Prerequisites

- Doxygen (version 1.9.1 or newer recommended)
- PlatformIO project structure
- Windows OS (for the included batch script) or adapt commands for other platforms

## Installation

1. **Install Doxygen**:
   - Download from [Doxygen's official website](http://www.doxygen.nl/download.html)
   - Follow installation instructions for your operating system
   - Ensure Doxygen is added to your system PATH

2. **Optional: Install Graphviz** (for better diagrams):
   - Download from [Graphviz's official website](https://graphviz.org/download/)
   - Ensure the `dot` executable is in your PATH

## Generating Documentation

### Using the Windows Batch Script

1. Open a command prompt in the project root directory
2. Run the included batch script:
```
windows-generate-docs.bat
```
3. The script will:
   - Check if Doxygen is installed
   - Create a `docs` directory if needed
   - Run Doxygen using the configuration in `Doxyfile`
   - Open the generated documentation in your default browser

### Manual Generation

If you're not using Windows or prefer to run commands manually:

1. Open a terminal in the project root directory
2. Run the following command: windows-generate-docs.bat
```
doxygen Doxyfile
```
3. The documentation will be generated in the `docs/html` directory
4. Open `docs/html/index.html` in a web browser to view the documentation

## Documentation Structure

The generated documentation includes:
- Class hierarchies for sensor components, managers, and tasks
- Detailed descriptions of functions and methods
- Diagrams showing class relationships
- Source code browser with syntax highlighting
- Alphabetical index of all elements

## Adding Documentation to Your Code

The Doxyfile is configured to extract documentation from specially formatted comments. Here's how to document 
your code:

```cpp
/**
 * @brief Brief description of class or function
 * 
 * Detailed description that can span
 * multiple lines
 * 
 * @param paramName Description of parameter
 * @return Description of return value
 */
```

Example from Constants.h:
```cpp
/**
 * @file Constants.h
 * @brief System-wide constants and configuration values
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @defgroup constants System Constants
 * @brief Global constants and configuration values
 * @{
 */
```

## Customizing Documentation

The Doxyfile contains all configuration options for documentation generation. Key settings include:

- `PROJECT_NAME` and `PROJECT_NUMBER`: Project details
- `INPUT`: Source directories to scan
- `EXTRACT_PRIVATE`: Whether to include private members
- `HAVE_DOT`: Enable/disable diagram generation
- `OUTPUT_DIRECTORY`: Where to store generated files

To modify these settings, edit the Doxyfile with a text editor.

## Troubleshooting

- **"Doxygen is not installed"**: Ensure Doxygen is installed and in your PATH
- **Missing diagrams**: Install Graphviz and ensure the `dot` executable is in your PATH
- **Empty documentation**: Check that your source files use proper Doxygen comment format
- **Build errors**: Check the Doxygen output for specific error messages

## Further Resources

- [Doxygen Manual](https://www.doxygen.nl/manual/)
- [Doxygen Comment Examples](https://www.doxygen.nl/manual/docblocks.html)