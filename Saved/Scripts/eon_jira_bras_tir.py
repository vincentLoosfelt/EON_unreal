# EON — bras armé de Jira entièrement droit dans l'axe du tir (Vincent, 25/11). Ajoute à la sortie de
# SandboxCharacter_CMC_ABP deux Transform (Modify) Bone (rotation remplacée, repère monde) : upperarm_r (variable
# EONTirEpaule) puis lowerarm_r (EONTirCanon), poids EONTirAlpha. AEONCharacter::Tick calcule ces rotations.
# Le canon dessiné n'est pas dans l'axe de son os lowerarm_r : axes mesurés sur Saved/Jira/Jira_UEFN.blend (sommets
# pesant > 0,8 sur l'os, axe principal), passés dans le repère de l'os par la pose de référence de SK_Jira_UEFN :
#   haut du bras (upperarm_r) : (-0.9925, -0.1014, -0.0677), 7° de l'os ;
#   canon (lowerarm_r)        : (-0.8287,  0.5273, -0.1875), 34° de l'os.
# Bout du canon : 1,161 × la longueur coude-main depuis le coude (flash des tirs, AEONCharacter::OnShot).
# Lancer : UnrealEditor-Cmd <EON.uproject complet> -run=pythonscript -script=<ce fichier>
import unreal
abp = unreal.load_asset("/Game/Blueprints/SandboxCharacter_CMC_ABP")
r = unreal.EONStatics.add_arm_aim(abp, "EONTirAlpha", "upperarm_r", "EONTirEpaule", "lowerarm_r", "EONTirCanon")
unreal.log("EON_BRAS_TIR " + r)
unreal.log("EON_BRAS_TIR sauvegarde %s" % unreal.EditorAssetLibrary.save_loaded_asset(abp, False))
