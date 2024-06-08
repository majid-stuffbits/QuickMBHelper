import pandas as pd
import os

def update_register_map(excel_file_path, c_file_path):
    # Read the Excel file
    df = pd.read_excel(excel_file_path)

    # Generate macro definitions
    macro_definitions = ""
    for index, row in df.iterrows():
        macro_definitions += f"#define {row['Macro Name']: <30} {row['Address']: >10}\n"

    # Get the last register address macro
    last_register_address_macro = df.iloc[-1]['Macro Name']

    macro_definitions += f"#define MB_LAST_REGISTER_ADDRESS {" " * (35 - len(last_register_address_macro))}{last_register_address_macro}\n"

    # Generate register map array
    register_map_array = "static const RegisterMap_t registerMap[] =\n{\n"
    for index, row in df.iterrows():
        min_lim_ptr = "NULL" if pd.isna(row['Min Limit Pointer']) else row['Min Limit Pointer']
        max_lim_ptr = "NULL" if pd.isna(row['Max Limit Pointer']) else row['Max Limit Pointer']
        register_map_array += (
            f"    {{{row['Macro Name']}, {row['Data Pointer']}, "
            f"sizeof({row['Data Pointer']}), {row['Access Type']}, {row['Data Type']}, "
            f"{min_lim_ptr}, {max_lim_ptr}}},\n"
        )
    register_map_array += "};\n"

    # Read the original C code file
    with open(c_file_path, 'r') as file:
        c_code = file.read()

    # Replace the macro definitions and register map array
    c_code = c_code.split("// MACRO DEFINITIONS START")[0] + \
             "// MACRO DEFINITIONS START\n" + \
             macro_definitions + \
             "// MACRO DEFINITIONS END" + \
             c_code.split("// MACRO DEFINITIONS END")[1].split("// REGISTER MAP ARRAY START")[0] + \
             "// REGISTER MAP ARRAY START\n" + \
             register_map_array + \
             "// REGISTER MAP ARRAY END" + \
             c_code.split("// REGISTER MAP ARRAY END")[1]

    # Write the updated C code back to the file
    with open(c_file_path, 'w') as file:
        file.write(c_code)

# # Example usage
# excel_file_path = r"C:\Data_D_Drive\GITHUB\ModbusAutogen\registermap.xlsx"
# c_file_path = r"C:\Data_D_Drive\GITHUB\ModbusAutogen\modbus_slave_user.c"


# update_register_map(excel_file_path, c_file_path)


def main():
    # Get paths from user input
    excel_file_path = input("Enter the path to the Excel file: ").strip().strip('"')
    c_file_path = input("Enter the path to the C code file: ").strip().strip('"')

    # Normalize paths
    excel_file_path = os.path.normpath(excel_file_path)
    c_file_path = os.path.normpath(c_file_path)

    # Check if the paths are valid
    if not os.path.exists(excel_file_path):
        print("Invalid Excel file path.")
        return
    if not os.path.exists(c_file_path):
        print("Invalid C code file path.")
        return

    # Update register map
    update_register_map(excel_file_path, c_file_path)
    print("Register map updated successfully.")

if __name__ == "__main__":
    main()