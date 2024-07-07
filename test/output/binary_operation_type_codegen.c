int main(void) {
	float a = 5.0;
	int b = 2;
	float c = (a + 2);
	a = (((a + b) * a) - 1);
	a = ((a + (b * a)) - 1);
	a = (a + (b * (a - 1)));
	return 0;
}