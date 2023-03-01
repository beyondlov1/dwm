int 
cmp( Client * c1,  Client *c2)
{
	if (c1->tags - c2->tags == 0)
	{
		return c1->win - c2->win;
	}
	else
	{
		return c1->tags - c2->tags;
	}
}

void 
sort(void *clist[], size_t n, int (*cmp)(const void *, const void *))
{
	int i;
	for (size_t i = 0; i < n; i++)
	{
		for (size_t j = i + 1; j < n; j++)
		{
			if (cmp(clist[j], clist[i]) < 0)
			{
				void *tmp = clist[i];
				clist[i] = clist[j];
				clist[j] = tmp;
			}
		}
	}
}
