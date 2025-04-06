import re

# Input CSV file
input_file = "feed.csv"  # Replace with your actual input file name
output_file = "feed2.csv"  # Replace with your desired output file name

# Read the input CSV content
with open(input_file, "r") as file:
    content = file.read()

# Replace parentheses with square brackets
updated_content = re.sub(r"\(([^)]+)\)", r"[\1]", content)

# Write the updated content to the output file
with open(output_file, "w") as file:
    file.write(updated_content)

print(f"Updated CSV has been saved to {output_file}")
