
void
gapgridsorted(Monitor *m) 
{
	unsigned int n, cols, rows, cn, rn, i, cx, cy, cw, ch;
	Client *c;

	for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) ;
	if(n == 0)
		return;

	/* grid dimensions */
	for(cols = 0; cols <= n/2; cols++)
		if(cols*cols >= n)
			break;
	if(n == 5) /* set layout against the general calculation: not 1:2:2, but 2:3 */
		cols = 2;
	rows = n/cols;

	/* window geometries */
	cw = cols ? m->ww / cols : m->ww;
	cn = 0; /* current column number */
	rn = 0; /* current row number */
	Client *sorted[n];
	for(i = 0, c = nexttiled(m->clients); c; i++, c = nexttiled(c->next)) {
		sorted[i] = c;
	}
	sort((void **)sorted, n, (int (*)(const void *, const void *))cmp);

	for(i = 0; i < n; i++) {
		c = sorted[i];
		if(i/rows + 1 > cols - n%cols)
			rows = n/cols + 1;
		ch = rows ? m->wh / rows : m->wh;
		cx = m->wx + cn*cw;
		cy = m->wy + rn*ch;
		int tmpbw = c->bw;
		c->bw = borderpx;
		resize(c, cx, cy, cw - 2 * c->bw - c->mon->gap->gappx, ch - 2 * c->bw - c->mon->gap->gappx, False);
		c->bw = tmpbw;
		rn++;
		if(rn >= rows) {
			rn = 0;
			cn++;
		}
	}
}
