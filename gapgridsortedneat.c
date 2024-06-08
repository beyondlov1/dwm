
void
gapgridsortedneat(Monitor *m) 
{


	unsigned int n, cols, rows, cn, rn, i, cx, cy, cw, ch = 0;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) ;
	if(n == 0)
		return;

	/* grid dimensions */
	int rcanadidates[n];
	int j = 0;
	rows = 1;
	for (; rows <= n; rows++)
	{
		cols = n/rows + (n%rows > 0 ? 1: 0);
		if (rows * 1.0 / cols >= 1 && rows * 1.0/ cols <= 2)
		{
			rcanadidates[j] = rows;	
			j ++;
		}
	}

	rows = rcanadidates[(j-1)/2];
	cols = n / rows + (n % rows > 0 ? 1 : 0);

	/* window geometries */
	cw = 0;
	cn = 0; /* current column number */
	rn = 0; /* current row number */
	Client *sorted[n];
	for(i = 0, c = nexttiled(m->clients); c; i++, c = nexttiled(c->next)) {
		sorted[i] = c;
	}
	sort((void **)sorted, n, (int (*)(const void *, const void *))cmp);

	for(i = 0; i < n; i++) {
		c = sorted[i];
		rn = i % rows;
		cn = i / rows;
		int lastch = ch;
		int lastcw = cw;
		if (cn == cols - 1 && rn == 0 && i == n - 1)
		{
			// 最后一列的唯一一个
			ch = m->wh;
		}else if(cn == cols-1 && i == n-1 ){
			// 最后一列的最后一个
			ch = m->wh - cy - lastch;
		}else{
			ch = rows ? m->wh / rows : m->wh;
		}
		cw = m->ww / cols;
		cx = m->wx + cn * lastcw;
		cy = m->wy + rn * lastch;
		int tmpbw = c->bw;
		c->bw = borderpx;
		resize(c, cx, cy, cw - 2 * c->bw - c->mon->gap->gappx, ch - 2 * c->bw - c->mon->gap->gappx, False);
		c->bw = tmpbw;
	}
}
