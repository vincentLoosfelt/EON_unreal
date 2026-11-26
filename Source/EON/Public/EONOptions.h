// EON — options du joueur (menu pause > Options) : huit onglets comme dans les grands jeux.
// Les valeurs d'EON vont dans GameUserSettings.ini, section [/Script/EON.EONOptions] ; l'affichage
// et les groupes de qualité passent par UGameUserSettings. Tout est réappliqué à chaque carte.
// Mises à l'échelle : TSR (moteur), DLSS / DLAA (plugin NVIDIA), FSR (plugin AMD), XeSS (plugin Intel),
// une seule active à la fois ; aucune par défaut (TSR).
#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UWorld;
class UObject;
class UInputMappingContext;
class UInputAction;
struct FEONGameFeelRow;

namespace EONOptions
{
	/** Section : intertitre non sélectionnable (ex. « TOUCHES » et ses deux colonnes). */
	enum class EKind : uint8 { Choice, Slider, Toggle, Action, Bind, Section };

	/** Une ligne d'un onglet, prête à dessiner. */
	struct FRow
	{
		FString Label;
		FString Value;
		/** Aide du panneau de droite (les « \n » séparent les paragraphes). */
		FString Help;
		EKind Kind = EKind::Choice;
		/** Curseur : remplissage 0..1. */
		float Fill = 0.f;
		/** Interrupteur : OUI. */
		bool bOn = false;
		/** Ligne grisée (ex. qualité DLSS quand le DLSS n'est pas choisi) : l'aide dit pourquoi. */
		bool bEnabled = true;
		/** Lignes de touches : clavier-souris et manette (vide = aucune), conflit = même touche ailleurs.
		 *  Section des touches : titres des deux colonnes. */
		FString KeyKb, KeyPad;
		bool bHasKb = false, bHasPad = false;
		bool bConflictKb = false, bConflictPad = false;
		/** Lignes qui partagent la touche en conflit, colonne par colonne (pour l'aide). */
		FString ConflictKbWith, ConflictPadWith;
		/** Aperçu spécial dans le panneau d'aide : 1 mire de luminosité. */
		int32 Preview = 0;
	};

	constexpr int32 NumTabs = 8;
	FString TabName(int32 Tab);
	TArray<FRow> Rows(int32 Tab);

	/** Flèches gauche / droite sur une ligne. */
	void Change(UWorld* W, int32 Tab, int32 Row, int32 Dir);
	/** Curseur cliqué ou glissé à la souris : la valeur prend la fraction T (0..1) de sa plage. */
	void SetFraction(UWorld* W, int32 Tab, int32 Row, float T);
	/** Entrée sur une ligne (bPad : colonne manette). Renvoie true si la ligne attend maintenant une touche. */
	bool Activate(UWorld* W, int32 Tab, int32 Row, bool bPad);
	/** Touche choisie pendant une réassignation (bPad : colonne manette). */
	void Rebind(UWorld* W, int32 Tab, int32 Row, bool bPad, const FKey& Key);

	/** À chaque carte : moteur, mise à l'échelle, post-process, son, limite d'images en arrière-plan. */
	void ApplyWorld(UWorld* W);
	/** Choix du joueur appliqués au banc de game feel (tremblements, chiffres, visée assistée, volumes, caméra...). */
	void ApplyFeel(FEONGameFeelRow& F);

	// --- Lus par le jeu
	/** 0 automatique, 1 clavier-souris, 2 manette. */
	int32 Device();
	/** 0 Xbox, 1 PlayStation, 2 Nintendo. */
	int32 PadStyle();
	bool RunToggle();
	bool StickFire();
	bool HitstopOn();
	bool ShowFps();
	float HudOpacity();
	float HudScale();
	bool MapRotates();
	/** 0 normale, 1 grande, 2 très grande (dialogues). */
	int32 TextSize();
	/** Vitesse d'apparition du texte des dialogues (1 = normale, 0 = instantanée). */
	float TextSpeed();
	bool HighContrast();
	bool ReduceFlashes();
	/** Tir et Onde : un appui démarre, un appui arrête. */
	bool HoldToggle();
	/** Course au clavier (Maj gauche par défaut). */
	FKey RunKey();
	/** Zone morte des sticks choisie (0 = aucune, réglage d'origine), en fraction de la course. */
	float StickDeadZone();
	bool EasyMode();
	/** Mode facile : moitié des dégâts reçus, le reste gardé pour le coup suivant (deux coups d'un cœur = un cœur). */
	int32 EasyDamage(int32 Raw);

	/** Contexte d'entrées du joueur : copie d'IMC_EON avec ses touches réassignées et sa zone morte. */
	UInputMappingContext* BuildMappingContext(UObject* Outer, const UInputMappingContext* Source);
	/** Nom court d'une touche (ESPACE, CLIC G, LB, R1, ZL...) selon le style de manette ; symboles PlayStation en jetons PS_*. */
	FString KeyLabel(const FKey& K);
	/** Touche (clavier ou manette) de la première ligne du contexte pour une action, vide si aucune. */
	FString ActionLabel(const UInputMappingContext* Ctx, const UInputAction* Action, bool bPad);
}
