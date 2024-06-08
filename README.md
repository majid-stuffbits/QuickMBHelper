# QuickMBHelper

QuickMBHelper is a Python script designed to automate the process of updating the register map and macro definitions in a C code file based on data provided in an Excel file.

## Features

- Reads register map data from an Excel file.
- Updates macro definitions and the register map array in a C code file accordingly.
- Provides a simple graphical user interface for selecting input files and executing the update process.

## Requirements

- Python 3.x
- pandas library
- Tkinter library (for GUI)

## Usage

1. Ensure Python 3.x is installed on your system.
2. Install the required libraries by running:
3. Run the script using Python:
4. Follow the on-screen instructions to select the Excel file containing register map data and the C code file to be updated.

## File Structure

- `modbus_autogen.py`: Main Python script for updating the register map.
- `registermap.xlsx`: Sample Excel file containing register map data (modify as needed).
- `modbus_slave_user.c`: Sample C code file to be updated (modify as needed).

