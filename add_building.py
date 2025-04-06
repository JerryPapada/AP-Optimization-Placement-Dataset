import pandas as pd
import json

# Load the simulation results CSV
simulation_df = pd.read_csv("cleaned_output.csv", delimiter=";")
simulation_df.columns = simulation_df.columns.str.strip()  # Strip spaces

# Debugging: Print column names and a sample of the dataframe
print("Simulation Data Columns:", simulation_df.columns.tolist())
print("Sample Data:")
print(simulation_df.head())

# Load layout file
layout_df = pd.read_csv("/home/gerpapa/ns3/ns-3-dev/scratch/processed_scenarios.csv", delimiter=";")
layout_df.columns = layout_df.columns.str.strip()  # Strip spaces

# Debugging: Print layout file data
print("Layout Data:")
print(layout_df.head())

# Map layout data into a dictionary for efficient lookup
layout_dict = {
    filename.strip(): {
        "n_rooms_x": row["n_rooms_x"],
        "n_rooms_y": row["n_rooms_y"],
        "ap_placement": row["ap_placement"],
        "ap_positions": json.loads(row["ap_positions"]),
        "mmwave_positions": json.loads(row["mmwave_positions"]),
        "x_min": row["x_min"],
        "x_max": row["x_max"],
        "y_min": row["y_min"],
        "y_max": row["y_max"],
        "z_min": row["z_min"],
        "z_max": row["z_max"],
    }
    for index, row in layout_df.iterrows()
    for filename in row["layout_name"].split(";")  # Use layout_name for filenames
}

# Debugging: Print the layout dictionary
print("Layout Dictionary:")
for key, value in layout_dict.items():
    print(f"{key}: {value}")

# Add layout information to simulation data
def add_layout_info(row):
    filenames = row["SourceFile"].split(";")
    for filename in filenames:
        filename = filename.strip()
        if filename in layout_dict:
            building_info = layout_dict[filename]
            for key, value in building_info.items():
                row[key] = value
        #else:
            #print(f"Filename {filename} not found in layout dictionary!")
    return row

# Apply the function to add layout info to each row
simulation_df = simulation_df.apply(add_layout_info, axis=1)

# Save the updated dataframe to a new CSV
simulation_df.to_csv("updated_simulation_results.csv", sep=";", index=False)

# Debugging: Print the first few rows of the updated dataframe
print("Updated Simulation Data:")
print(simulation_df.head())
