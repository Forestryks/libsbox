#include <bits/stdc++.h>
using namespace std;

using ll = long long;

ll rnd(ll a, ll b) {
	static random_device rd;
	static mt19937 mt(rd());
	return mt() % (b - a + 1) + a;
}

int main(int argc, char *argv[]) {
	ios_base::sync_with_stdio(false); cin.tie(0); cout.tie(0);
	int n = atoi(argv[1]);
	int m = atoi(argv[2]);
	vector<int> a(n);
	for (int i = 0; i < n; ++i) {
		a[i] = rnd(1, m);
	}

	cout << n << ' ' << m << endl;
	for (int i = 0; i < n; ++i) {
		if (i) cout << ' ';
		cout << a[i];
	}
	cout << endl;
}
