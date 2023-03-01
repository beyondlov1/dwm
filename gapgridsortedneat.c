
void
gapgridsortedneat(Monitor *m) 
{
	unsigned int n, cols, rows, cn, rn, i, cx, cy, cw, ch = 0;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) ;
	if(n == 0)
		return;

	/* grid dimensions */
	rows = 2;
	cols = n/rows + (n%rows > 0 ? 1: 0);

	/* window geometries */
	cw = 0;
	cn = 0; /* current column number */
	rn = 0; /* current row number */
	Client *sorted[n];
	for(i = 0, c = nexttiled(m->clients); c; i++, c = nexttiled(c->next)) {
		sorted[i] = c;
	}
	sort(sorted, n, cmp);

	for(i = 0; i < n; i++) {
		c = sorted[i];
		rn = i % rows;
		cn = i / rows;
		if (cn == cols - 1 && i == n - 1 && i % rows == 0)
		{
			// 最后一列的唯一一个
			ch = m->wh;
		}else if(cn == cols-1 && i == n-1 ){
			// 最后一列的最后一个
			ch = m->wh - cy - ch;
		}else{
			ch = rows ? m->wh / rows : m->wh;
		}
		cw = m->ww / cols;
		cx = m->wx + cn * cw;
		cy = m->wy + rn * ch;
		int tmpbw = c->bw;
		c->bw = borderpx;
		resize(c, cx, cy, cw - 2 * c->bw - c->mon->gap->gappx, ch - 2 * c->bw - c->mon->gap->gappx, False);
		c->bw = tmpbw;
	}
}
