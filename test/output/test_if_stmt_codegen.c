int main(void)
{
	int x = 0;
	int pow = 3;
	if (0 <= x && x > 1)
	{
		pow = 1;
	}
	else if (x < 1)
	{
		pow = 2;
	}
	else
	{
		pow = pow * pow;
		pow = 2 * pow;
	}
	return 0;
}