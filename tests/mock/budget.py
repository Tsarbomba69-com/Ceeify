from datetime import datetime


class ExpenseTracker:
    def __init__(self, total_budget):
        self.total_budget = total_budget
        self.daily_expenses = []  # List of expenses to be applied every day
        self.days_count = 14

    def calculate_duration(self, start_date_str, end_date_str):
        date_format = "%Y-%m-%d"
        start = datetime.strptime(start_date_str, date_format)
        end = datetime.strptime(end_date_str, date_format)
        # Calculate the total days for the trip
        self.days_count = (end - start).days + 1
        print(f"Projecting for: {self.days_count} days")

    def add_daily_expense(self, name, daily_amount):
        self.daily_expenses.append({"name": name, "amount": daily_amount})

    def get_summary(self):
        # Calculate total cost based on (daily_amount * total_days)
        total_spent = sum(item['amount'] * self.days_count for item in self.daily_expenses)
        remaining = self.total_budget - total_spent

        print(f"\n--- Projection for {self.days_count} Days ---")
        print(f"{'Item':<15} | {'Daily':<10} | {'Total Trip Cost':<15}")
        print("-" * 45)

        for item in self.daily_expenses:
            trip_total = item['amount'] * self.days_count
            print(f"{item['name']:<15} | {item['amount']:>10,.2f} | {trip_total:>15,.2f}")

        print("-" * 45)
        print(f"Total Budget:      {self.total_budget:,.2f}")
        print(f"Total Projected:   {total_spent:,.2f}")
        print(f"Remaining Balance: {remaining:,.2f}")

        if remaining < 0:
            print("Warning: These daily habits will put you over budget!")

# Usage
tracker = ExpenseTracker(99_000)
tracker.calculate_duration("2026-02-18", "2026-02-28")  # 14 days

# These are now treated as daily costs
tracker.add_daily_expense("🚕", 300)  # 300 * 14
tracker.add_daily_expense("💧", 200 * 2)  # 200 * 14
tracker.add_daily_expense("🥤", 500)  # 500 * 14
tracker.add_daily_expense("🍪", 500)

tracker.get_summary()