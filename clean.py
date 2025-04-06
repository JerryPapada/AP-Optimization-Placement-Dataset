import pandas as pd

# Input and output file paths
input_file = "feed2.csv"  # Replace with your actual file name
output_file = "cleaned_output.csv"

# Load the CSV file
df = pd.read_csv(input_file, sep=';')

# Clean the SourceFile column
df['SourceFile'] = df['SourceFile'].str.replace(r'simulation_results_|\.csv|_\d+\.csv', '', regex=True)

# Save the cleaned data to a new file
df.to_csv(output_file, index=False, sep=';')

print(f"Cleaned file saved as: {output_file}")
