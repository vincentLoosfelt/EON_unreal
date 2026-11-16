// EON — génération des salles en tuiles. Portage ligne à ligne de genTiles / comTiles / buildTraining (proto v0.60.1).
#include "EONTileGrid.h"

namespace
{
	using ET = EEONTile;

	/** R(a,b) du proto : a + floor(random × (b − a + 1)), bornes incluses (et même formule si b < a). */
	int32 R(FRandomStream& Rng, int32 A, int32 B) { return A + FMath::FloorToInt(Rng.GetFraction() * (B - A + 1)); }
	template <typename V> V Pick(FRandomStream& Rng, std::initializer_list<V> L)
	{
		const int32 I = FMath::Min((int32)L.size() - 1, FMath::FloorToInt(Rng.GetFraction() * L.size()));
		return *(L.begin() + I);
	}
	const uint8 DoorOrder[4] = { EONDoor::N, EONDoor::S, EONDoor::E, EONDoor::W };
	const FIntPoint Dirs[4] = { FIntPoint(0, -1), FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(-1, 0) };

	struct FBuilder
	{
		FEONTileGrid& G;
		int32 W, H;
		explicit FBuilder(FEONTileGrid& InG) : G(InG), W(InG.W), H(InG.H) {}
		bool Inner(int32 X, int32 Y) const { return X > 0 && Y > 0 && X < W - 1 && Y < H - 1; }
		void Set(int32 X, int32 Y, ET V)
		{
			if (Inner(X, Y)) { G.T[Y * W + X] = (uint8)V; if (V != ET::Pit) { G.Gap.Remove(Y * W + X); } }
		}
		void Rect(int32 X0, int32 Y0, int32 RW, int32 RH, ET V, bool bGap = false)
		{
			for (int32 Y = Y0; Y < Y0 + RH; ++Y)
			{
				for (int32 X = X0; X < X0 + RW; ++X)
				{
					Set(X, Y, V);
					if (bGap && Inner(X, Y)) { G.Gap.Add(Y * W + X); }
				}
			}
		}
	};

	void InitBox(FEONTileGrid& G, int32 W, int32 H, ET Inside)
	{
		G.W = W; G.H = H;
		G.T.SetNumUninitialized(W * H);
		G.Gap.Reset();
		for (int32 Y = 0; Y < H; ++Y)
		{
			for (int32 X = 0; X < W; ++X)
			{
				G.T[Y * W + X] = (uint8)((X == 0 || Y == 0 || X == W - 1 || Y == H - 1) ? ET::Wall : Inside);
			}
		}
	}

	void PutDoors(FEONTileGrid& G)
	{
		for (uint8 D : DoorOrder)
		{
			if (!(G.Doors & D)) { continue; }
			FIntPoint P[2];
			FEONTileGrid::DoorTiles(D, G.W, G.H, P);
			for (const FIntPoint& Q : P) { G.T[Q.Y * G.W + Q.X] = (uint8)ET::Door; }
		}
	}
}

FIntPoint FEONTileGrid::ShapeSize(FName Shape)
{
	if (Shape == TEXT("large")) { return FIntPoint(40, 26); }
	if (Shape == TEXT("carree")) { return FIntPoint(32, 32); }
	if (Shape == TEXT("small")) { return FIntPoint(24, 16); }
	if (Shape == TEXT("corH")) { return FIntPoint(50, 9); }
	if (Shape == TEXT("corV")) { return FIntPoint(12, 32); }
	return FIntPoint(32, 20);
}

void FEONTileGrid::DoorTiles(uint8 D, int32 W, int32 H, FIntPoint Out[2])
{
	const int32 Cx = W >> 1, Cy = H >> 1;
	if (D == EONDoor::N) { Out[0] = FIntPoint(Cx - 1, 0); Out[1] = FIntPoint(Cx, 0); }
	else if (D == EONDoor::S) { Out[0] = FIntPoint(Cx - 1, H - 1); Out[1] = FIntPoint(Cx, H - 1); }
	else if (D == EONDoor::W) { Out[0] = FIntPoint(0, Cy - 1); Out[1] = FIntPoint(0, Cy); }
	else { Out[0] = FIntPoint(W - 1, Cy - 1); Out[1] = FIntPoint(W - 1, Cy); }
}

void FEONTileGrid::GetDoorTiles(uint8 D, FIntPoint Out[2]) const
{
	const int32 I = DoorIndex(D);
	if (DoorAt.IsValidIndex(I) && DoorAt[I].X >= 0)
	{
		Out[0] = DoorAt[I];
		Out[1] = DoorAt[I] + ((D == EONDoor::N || D == EONDoor::S) ? FIntPoint(1, 0) : FIntPoint(0, 1));
		return;
	}
	DoorTiles(D, W, H, Out);
}

FEONTileGrid FEONTileGrid::GenTiles(FName RoleIn, FName Shape, uint8 Doors, FRandomStream& Rng, int32 FloorN)
{
	const FString Role = RoleIn.ToString();
	const FIntPoint S = ShapeSize(Shape);
	const int32 W = S.X, H = S.Y, IW = W - 2, IH = H - 2, Cx = W >> 1, Cy = H >> 1;
	const bool bN = Doors & EONDoor::N, bS = Doors & EONDoor::S, bE = Doors & EONDoor::E, bW = Doors & EONDoor::W;

	for (int32 Attempt = 0; Attempt < 16; ++Attempt)
	{
		FEONTileGrid G;
		G.Doors = Doors;
		InitBox(G, W, H, ET::Floor);
		FBuilder B(G);
		FString Feat;
		const bool bCor = Shape == TEXT("corH") || Shape == TEXT("corV");

		// --- forme ---
		if (!bCor && (Role == TEXT("COMBAT") || Role == TEXT("TENSION") || Role == TEXT("CALME")) && Attempt < 12)
		{
			const FString M = Pick<const TCHAR*>(Rng, { TEXT("none"), TEXT("none"), TEXT("cross"), TEXT("L"), TEXT("donut"), TEXT("notch") });
			if (M == TEXT("cross"))
			{
				const int32 A = FMath::FloorToInt(IW * 0.25f), Bb = FMath::FloorToInt(IH * 0.28f);
				B.Rect(1, 1, A, Bb, ET::Wall); B.Rect(W - 1 - A, 1, A, Bb, ET::Wall); B.Rect(1, H - 1 - Bb, A, Bb, ET::Wall); B.Rect(W - 1 - A, H - 1 - Bb, A, Bb, ET::Wall);
			}
			else if (M == TEXT("L"))
			{
				const int32 A = IW / 2 - 2, Bb = IH / 2 - 1, C = R(Rng, 0, 3);
				B.Rect((C & 1) ? W - 1 - A : 1, (C & 2) ? H - 1 - Bb : 1, A, Bb, ET::Wall);
			}
			else if (M == TEXT("donut"))
			{
				const int32 A = FMath::Max(2, FMath::FloorToInt(IW * 0.22f)), Bb = FMath::Max(2, FMath::FloorToInt(IH * 0.25f));
				B.Rect((W - A) >> 1, (H - Bb) >> 1, A, Bb, ET::Wall);
			}
			else if (M == TEXT("notch"))
			{
				for (int32 K = R(Rng, 2, 4); K > 0; --K)
				{
					const int32 L = R(Rng, 2, 3), Side = R(Rng, 0, 3);
					if (Side < 2)
					{
						const int32 A1 = R(Rng, 3, Cx - 4), A2 = R(Rng, Cx + 3, W - 4);
						const int32 X = Pick<int32>(Rng, { A1, A2 });
						B.Rect(X, Side ? H - 1 - L : 1, R(Rng, 1, 2), L, ET::Wall);
					}
					else
					{
						const int32 A1 = R(Rng, 3, FMath::Max(3, Cy - 3)), A2 = R(Rng, Cy + 2, H - 4);
						const int32 Y = Pick<int32>(Rng, { A1, A2 });
						B.Rect(Side == 2 ? 1 : W - 1 - L, Y, L, R(Rng, 1, 2), ET::Wall);
					}
				}
			}
			Feat = M == TEXT("none") ? FString() : M;
		}

		auto AddFeat = [&Feat](const FString& O) { Feat += (Feat.IsEmpty() ? TEXT("") : TEXT("+")) + O; };

		// --- contenu ---
		if (bCor)
		{
			const bool bHz = Shape == TEXT("corH");
			const int32 L = bHz ? W : H, Sx = bHz ? H : W;
			auto RO = [&](int32 A, int32 Bv, int32 La, int32 Lb, ET V, bool bGap = false)
			{
				if (bHz) { B.Rect(A, Bv, La, Lb, V, bGap); } else { B.Rect(Bv, A, Lb, La, V, bGap); }
			};
			const FString O = Role == TEXT("CORRIDOR") ? FString(TEXT("crates"))
				: FString(Pick<const TCHAR*>(Rng, { TEXT("gaps"), TEXT("gaps"), TEXT("side"), TEXT("crates"), TEXT("holes") }));
			Feat = O;
			if (O == TEXT("gaps"))
			{
				const int32 N = R(Rng, 1, 2);
				const int32 P1 = R(Rng, FMath::FloorToInt(L * 0.25f), FMath::FloorToInt(L * 0.4f));
				RO(P1, 1, 2, Sx - 2, ET::Pit, true);
				if (N > 1) { RO(R(Rng, FMath::FloorToInt(L * 0.6f), FMath::FloorToInt(L * 0.72f)), 1, 2, Sx - 2, ET::Pit, true); }
			}
			else if (O == TEXT("side"))
			{
				int32 A = R(Rng, 5, 9);
				while (A < L - 8)
				{
					const int32 Len = R(Rng, 5, 9);
					const bool bTop = Rng.GetFraction() < 0.5f;
					RO(A, bTop ? 1 : Sx - 3, Len, 2, ET::Pit);
					A += Len + R(Rng, 2, 5);
				}
			}
			else if (O == TEXT("crates"))
			{
				for (int32 K = R(Rng, 5, 8); K > 0; --K)
				{
					const int32 A = R(Rng, 4, L - 5), Bv = R(Rng, 1, Sx - 2);
					if (bHz) { B.Set(A, Bv, ET::Crate); } else { B.Set(Bv, A, ET::Crate); }
				}
			}
			else
			{
				for (int32 K = R(Rng, 3, 5); K > 0; --K)
				{
					const int32 A = R(Rng, 5, L - 7);
					RO(A, R(Rng, 1, Sx - 3), R(Rng, 1, 2), R(Rng, 1, 2), ET::Pit);
				}
			}
		}
		else if (Role == TEXT("COMBAT"))
		{
			// Écart au proto (Vincent, 16/11) : plus de piliers 2×2 (« pillars », et ceux de « mix ») au milieu des salles de combat.
			const FString O = Pick<const TCHAR*>(Rng, { TEXT("holes"), TEXT("centre"), TEXT("crates"), TEXT("mix") });
			AddFeat(O);
			if (O == TEXT("holes") || O == TEXT("mix"))
			{
				for (int32 K = O == TEXT("mix") ? 2 : R(Rng, 2, 4); K > 0; --K) { B.Rect(R(Rng, 3, W - 5), R(Rng, 3, H - 5), R(Rng, 1, 3), R(Rng, 1, 2), ET::Pit); }
			}
			if (O == TEXT("centre"))
			{
				const int32 A = FMath::FloorToInt(IW * R(Rng, 22, 32) / 100.f), Bb = FMath::FloorToInt(IH * R(Rng, 28, 38) / 100.f);
				B.Rect(Cx - (A >> 1), Cy - (Bb >> 1), A, Bb, ET::Pit);
			}
			if (O == TEXT("crates"))
			{
				for (int32 K = R(Rng, 4, 8); K > 0; --K)
				{
					const int32 X = R(Rng, 3, W - 4), Y = R(Rng, 3, H - 4);
					B.Set(X, Y, ET::Crate);
					if (Rng.GetFraction() < 0.4f) { B.Set(X + 1, Y, ET::Crate); }
				}
			}
		}
		else if (Role == TEXT("TENSION"))
		{
			const FString O = Pick<const TCHAR*>(Rng, { TEXT("chasm"), TEXT("islands"), TEXT("islands"), TEXT("holes") });
			AddFeat(O);
			if (O == TEXT("chasm"))
			{
				const bool bVert = (bE || bW) && (Rng.GetFraction() < 0.75f || !(bN || bS));
				if (bVert)
				{
					const int32 Bw = R(Rng, 3, 5), X0 = R(Rng, FMath::FloorToInt(W * 0.3f), FMath::FloorToInt(W * 0.62f) - Bw);
					B.Rect(X0, 1, Bw, IH, ET::Pit);
					for (int32 K = R(Rng, 1, 2); K > 0; --K) { B.Rect(X0, R(Rng, 2, H - 4), Bw, 2, ET::Floor); }
				}
				else
				{
					const int32 Bh = R(Rng, 3, 4), Y0 = R(Rng, FMath::FloorToInt(H * 0.3f), FMath::FloorToInt(H * 0.62f) - Bh);
					B.Rect(1, Y0, IW, Bh, ET::Pit);
					for (int32 K = R(Rng, 1, 2); K > 0; --K) { B.Rect(R(Rng, 2, W - 4), Y0, 2, Bh, ET::Floor); }
				}
			}
			else if (O == TEXT("islands"))
			{
				B.Rect(1, 1, IW, IH, ET::Pit);
				B.Rect(Cx - 2, Cy - 2, 4, 4, ET::Floor);
				if (bN) { B.Rect(Cx - 1, 1, 2, Cy - 1, ET::Floor); }
				if (bS) { B.Rect(Cx - 1, Cy, 2, H - 1 - Cy, ET::Floor); }
				if (bW) { B.Rect(1, Cy - 1, Cx - 1, 2, ET::Floor); }
				if (bE) { B.Rect(Cx, Cy - 1, W - 1 - Cx, 2, ET::Floor); }
				for (int32 K = R(Rng, 2, 4); K > 0; --K)
				{
					const int32 Pw = R(Rng, 3, 5), Ph = R(Rng, 3, 4), Px = R(Rng, 2, W - 2 - Pw), Py = R(Rng, 2, H - 2 - Ph);
					B.Rect(Px, Py, Pw, Ph, ET::Floor);
					const int32 Pcx = Px + (Pw >> 1), Pcy = Py + (Ph >> 1);
					const int32 X0 = FMath::Min(Pcx, Cx), Len = FMath::Abs(Pcx - Cx) + 2;
					B.Rect(X0, Pcy, Len, 2, ET::Floor);
					B.Rect(Cx, FMath::Min(Pcy, Cy), 2, FMath::Abs(Pcy - Cy) + 2, ET::Floor);
					if (Len >= 8 && Rng.GetFraction() < 0.45f) { const int32 Gx = X0 + (Len >> 1) - 1; B.Rect(Gx, Pcy, 2, 2, ET::Pit, true); }
				}
			}
			else
			{
				for (int32 K = R(Rng, 6, 10); K > 0; --K) { B.Rect(R(Rng, 2, W - 4), R(Rng, 2, H - 4), R(Rng, 1, 3), R(Rng, 1, 2), ET::Pit); }
			}
		}
		else if (Role == TEXT("CALME"))
		{
			for (int32 X = R(Rng, 2, 4); X < W - 2; X += R(Rng, 3, 5)) { B.Set(X, 1, ET::Crate); }
			for (int32 K = R(Rng, 1, 3); K > 0; --K) { const int32 X = R(Rng, 3, W - 6), Y = R(Rng, 3, H - 5); B.Rect(X, Y, R(Rng, 1, 3), R(Rng, 1, 2), ET::Crate); }
			if (Rng.GetFraction() < 0.4f) { B.Rect(R(Rng, 3, W - 6), R(Rng, 3, H - 5), R(Rng, 2, 3), 2, ET::Pit); }
		}
		else if (Role == TEXT("DEPART"))
		{
			for (int32 X = 3; X < W - 3; X += 4) { if (FMath::Abs(X - Cx) > 2) { B.Set(X, 1, ET::Crate); } }
		}
		else if (Role == TEXT("OPPORTUNITE"))
		{
			B.Set(2, 2, ET::Crate); B.Set(W - 3, 2, ET::Crate); B.Set(2, H - 3, ET::Crate); B.Set(W - 3, H - 3, ET::Crate);
		}
		else if (Role == TEXT("ASCENSEUR"))
		{
			B.Rect(2, 2, 3, 2, ET::Pit); B.Rect(W - 5, 2, 3, 2, ET::Pit); B.Rect(2, H - 4, 3, 2, ET::Pit); B.Rect(W - 5, H - 4, 3, 2, ET::Pit);
			uint8 First = 0;
			for (uint8 D : DoorOrder) { if (Doors & D) { First = D; break; } }
			const uint8 Side = First ? EONDoor::Opp(First) : EONDoor::N;
			const int32 O = FloorN == 4 ? 3 : 0;   // étage 4 : le saule pousse plus au centre
			if (Side == EONDoor::N) { B.Rect(Cx - 2, 2 + O, 4, 3, ET::Tank); }
			else if (Side == EONDoor::S) { B.Rect(Cx - 2, H - 5 - O, 4, 3, ET::Tank); }
			else if (Side == EONDoor::W) { B.Rect(2 + O, Cy - 2, 3, 4, ET::Tank); }
			else { B.Rect(W - 5 - O, Cy - 2, 3, 4, ET::Tank); }
		}
		else if (Role == TEXT("SECURITE"))
		{
			B.Rect(Cx - 4, Cy - 4, 8, 8, ET::Pit, true);
			B.Rect(Cx - 2, Cy - 2, 4, 4, ET::Floor);
			for (int32 K = R(Rng, 2, 4); K > 0; --K) { const int32 A1 = R(Rng, 3, Cx - 7), A2 = R(Rng, Cx + 6, W - 4); const int32 X = Pick<int32>(Rng, { A1, A2 }), Y = R(Rng, 3, H - 4); B.Set(X, Y, ET::Crate); }
		}
		else if (Role == TEXT("MARCHAND"))
		{
			for (int32 Y = 4; Y < H - 3; Y += R(Rng, 2, 3)) { B.Set(1, Y, ET::Crate); B.Set(W - 2, Y, ET::Crate); }
			for (int32 K = R(Rng, 2, 4); K > 0; --K)
			{
				const int32 A1 = R(Rng, 3, FMath::Max(3, Cx - 6)), A2 = R(Rng, Cx + 5, W - 4);
				const int32 X = Pick<int32>(Rng, { A1, A2 }), Y = R(Rng, 5, H - 4);
				B.Rect(X, Y, R(Rng, 1, 2), R(Rng, 1, 2), ET::Crate);
			}
			B.Rect(Cx - 5, 1, 10, 4, ET::Floor);
		}
		else if (Role == TEXT("FAILLE"))
		{
			for (int32 K = R(Rng, 2, 3); K > 0; --K) { B.Rect(R(Rng, 5, W - 7), R(Rng, 4, H - 6), 2, 2, ET::Wall); }
			for (int32 K = R(Rng, 3, 5); K > 0; --K) { const int32 X = R(Rng, 5, W - 6), Y = R(Rng, 4, H - 5); B.Set(X, Y, ET::Crate); }
		}
		else if (Role == TEXT("EVENEMENT"))
		{
			for (int32 Y = 3; Y < H - 3; Y += R(Rng, 2, 3)) { B.Set(1, Y, ET::Crate); B.Set(W - 2, Y, ET::Crate); }
			for (int32 X = 3; X < W - 3; X += R(Rng, 3, 4)) { if (FMath::Abs(X - Cx) > 4) { B.Set(X, H - 2, ET::Crate); } }
			B.Rect(Cx - 3, 1, 6, 4, ET::Floor);
			if (Rng.GetFraction() < 0.5f) { B.Rect(R(Rng, 4, 7), R(Rng, 5, H - 6), 2, 2, ET::Pit); }
		}

		// --- portes + dégagement ---
		for (uint8 D : DoorOrder)
		{
			if (!(Doors & D)) { continue; }
			FIntPoint P[2];
			DoorTiles(D, W, H, P);
			for (const FIntPoint& Q : P) { G.T[Q.Y * W + Q.X] = (uint8)ET::Door; }
			if (D == EONDoor::N) { B.Rect(Cx - 2, 1, 4, 3, ET::Floor); }
			if (D == EONDoor::S) { B.Rect(Cx - 2, H - 4, 4, 3, ET::Floor); }
			if (D == EONDoor::W) { B.Rect(1, Cy - 2, 3, 4, ET::Floor); }
			if (D == EONDoor::E) { B.Rect(W - 4, Cy - 2, 3, 4, ET::Floor); }
		}
		if (Role == TEXT("OPPORTUNITE")) { B.Rect(Cx - 6, Cy - 2, 12, 4, ET::Floor); }
		if (Role == TEXT("ASCENSEUR") || Role == TEXT("DEPART")) { B.Rect(Cx - 3, Cy - 2, 6, 4, ET::Floor); }
		if (Role == TEXT("DEPART")) { B.Rect(Cx - 1, Cy - 1, 2, 2, ET::Tank); }
		if (Role == TEXT("SECURITE")) { B.Rect(Cx - 2, Cy - 2, 4, 4, ET::Floor); }

		// --- validation : toutes les portes reliées, au moins 45 % du sol et 40 tuiles accessibles ---
		auto Walk = [&G](int32 I) { const ET V = (ET)G.T[I]; return V == ET::Floor || V == ET::Door || (V == ET::Pit && G.Gap.Contains(I)); };
		uint8 First = 0;
		for (uint8 D : DoorOrder) { if (Doors & D) { First = D; break; } }
		bool bOk = true;
		if (First)
		{
			TArray<uint8> Seen; Seen.SetNumZeroed(W * H);
			FIntPoint S0[2]; DoorTiles(First, W, H, S0);
			TArray<int32> Q; Q.Add(S0[0].Y * W + S0[0].X); Seen[Q[0]] = 1;
			int32 Cnt = 0, Tot = 0;
			for (int32 I = 0; I < W * H; ++I) { if (G.T[I] == (uint8)ET::Floor) { ++Tot; } }
			while (Q.Num())
			{
				const int32 I = Q.Pop(EAllowShrinking::No);
				if (G.T[I] == (uint8)ET::Floor) { ++Cnt; }
				const int32 X = I % W, Y = I / W;
				for (const FIntPoint& Dd : Dirs)
				{
					const int32 Nx = X + Dd.X, Ny = Y + Dd.Y;
					if (Nx < 0 || Ny < 0 || Nx >= W || Ny >= H) { continue; }
					const int32 J = Ny * W + Nx;
					if (!Seen[J] && Walk(J)) { Seen[J] = 1; Q.Add(J); }
				}
			}
			for (uint8 D : DoorOrder)
			{
				if (!(Doors & D)) { continue; }
				FIntPoint P[2]; DoorTiles(D, W, H, P);
				for (const FIntPoint& Q2 : P) { if (!Seen[Q2.Y * W + Q2.X]) { bOk = false; } }
			}
			bOk = bOk && Cnt >= Tot * 0.45f && Cnt >= 40;
		}
		if (bOk) { G.Feat = Feat; return G; }
	}
	FEONTileGrid G;
	G.Doors = Doors;
	InitBox(G, W, H, ET::Floor);
	PutDoors(G);
	G.Feat = TEXT("plain");
	return G;
}

FEONTileGrid FEONTileGrid::ComTiles(uint8 Doors)
{
	FEONTileGrid G;
	G.Doors = Doors;
	const int32 W = 32, H = 20, Cx = W >> 1, Cy = H >> 1;
	InitBox(G, W, H, ET::Pit);
	auto F = [&G, W, H](int32 X, int32 Y) { if (X > 0 && Y > 0 && X < W - 1 && Y < H - 1) { G.T[Y * W + X] = (uint8)ET::Floor; } };
	for (int32 Y = 1; Y < H - 1; ++Y)
	{
		for (int32 X = 1; X < W - 1; ++X)
		{
			const float Dx = (X + 0.5f - Cx) / 6.3f, Dy = (Y + 0.5f - Cy) / 5.3f;
			if (Dx * Dx + Dy * Dy <= 1.f) { F(X, Y); }
		}
	}
	for (uint8 D : DoorOrder)
	{
		if (!(Doors & D)) { continue; }
		FIntPoint P[2]; DoorTiles(D, W, H, P);
		for (const FIntPoint& Q : P) { G.T[Q.Y * W + Q.X] = (uint8)ET::Door; }
		if (D == EONDoor::N) { for (int32 Y = 1; Y < Cy; ++Y) { F(Cx - 1, Y); F(Cx, Y); } for (int32 X = Cx - 2; X <= Cx + 1; ++X) { F(X, 1); F(X, 2); } }
		if (D == EONDoor::S) { for (int32 Y = Cy; Y < H - 1; ++Y) { F(Cx - 1, Y); F(Cx, Y); } for (int32 X = Cx - 2; X <= Cx + 1; ++X) { F(X, H - 2); F(X, H - 3); } }
		if (D == EONDoor::W) { for (int32 X = 1; X < Cx; ++X) { F(X, Cy - 1); F(X, Cy); } for (int32 Y = Cy - 2; Y <= Cy + 1; ++Y) { F(1, Y); F(2, Y); } }
		if (D == EONDoor::E) { for (int32 X = Cx; X < W - 1; ++X) { F(X, Cy - 1); F(X, Cy); } for (int32 Y = Cy - 2; Y <= Cy + 1; ++Y) { F(W - 2, Y); F(W - 3, Y); } }
	}
	for (int32 Y = Cy - 2; Y < Cy + 2; ++Y) { for (int32 X = Cx - 2; X < Cx + 2; ++X) { G.T[Y * W + X] = (uint8)ET::Tank; } }
	G.Feat = TEXT("com");
	return G;
}

const TArray<FIntPoint>& FEONTileGrid::TrainingPods()
{
	static const TArray<FIntPoint> Pods = { {3, 2}, {7, 2}, {11, 2}, {21, 2}, {11, 15}, {22, 15}, {17, 15} };
	return Pods;
}

FEONTileGrid FEONTileGrid::BuildTraining()
{
	FEONTileGrid G;
	const int32 W = 36, H = 20;
	G.Doors = EONDoor::N;
	InitBox(G, W, H, ET::Floor);
	for (int32 Y = 1; Y < H - 1; ++Y) { for (int32 X = 27; X < 29; ++X) { G.T[Y * W + X] = (uint8)ET::Pit; G.Gap.Add(Y * W + X); } }
	for (int32 Y = 12; Y < 14; ++Y) { for (int32 X = 3; X < 7; ++X) { G.T[Y * W + X] = (uint8)ET::Pit; } }
	for (const FIntPoint& P : TrainingPods())
	{
		for (int32 Y = P.Y; Y < P.Y + 3; ++Y) { for (int32 X = P.X; X < P.X + 2; ++X) { G.T[Y * W + X] = (uint8)ET::Tank; } }
	}
	PutDoors(G);
	G.Feat = TEXT("training");
	return G;
}
