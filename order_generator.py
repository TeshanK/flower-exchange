# get command line arguments for the number of rows to generate
# generate the orders and write them to a csv file

import csv
import random
import argparse

parser = argparse.ArgumentParser(description='Generate CSV orders')
parser.add_argument('--rows', type=int, default=100, help='Number of rows to generate')
parser.add_argument('--file', type=str, default='orders.csv', help='Output CSV file name')
args = parser.parse_args()

# format (id, [Rose, Lavender, Lotus, Tulip, Orchid], [1, 2], [10-1000](multiple
# of 10),double[1.00 - 1000.00])
def generate_order_and_append_to_csv(num_rows):
    with open(args.file, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(['id', 'flower', 'side', 'quantity', 'price'])
        for i in range(1, num_rows + 1):
            flower = random.choice(['Rose', 'Lavender', 'Lotus', 'Tulip', 'Orchid'])
            side = random.choice([1, 2])
            quantity = random.randint(1, 100) * 10
            price = round(random.uniform(1.00, 1000.00), 2)
            writer.writerow([i, flower, side, quantity, price])

if __name__ == "__main__":
    generate_order_and_append_to_csv(args.rows)
