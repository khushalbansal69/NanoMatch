import random
import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--orders", type=int, default=100000)
parser.add_argument("--output", type=str, default="orders.csv")
args = parser.parse_args()

random.seed(42)
base_price = 10000

with open(f"data/synthetic/{args.output}", "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["order_id", "side", "price", "quantity", "type"])

    for i in range(1, args.orders + 1):
        side = random.choice(["BUY", "SELL"])
        order_type = "LIMIT" if random.random() < 0.85 else "MARKET"
        price = base_price + random.randint(-50, 50) if order_type == "LIMIT" else 0
        quantity = random.randint(1, 500)
        writer.writerow([i, side, price, quantity, order_type])

        # Drift price slightly to simulate real market movement
        base_price += random.randint(-1, 1)

print(f"Generated {args.orders} orders -> data/synthetic/{args.output}")
