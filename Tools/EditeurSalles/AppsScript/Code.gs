/**
 * EON — Éditeur de salles, appli web Google (Apps Script).
 *
 * L'éditeur s'ouvre par une adresse web, connecté au compte Google, et travaille
 * directement dans le dossier Drive « EON_EditeurSalles » :
 *   EON_EditeurSalles/EditeurSalles.html   la page de l'éditeur (servie telle quelle)
 *   EON_EditeurSalles/Bibliotheque/        .eonasset, .eonapercu, Salle.eontheme, Textures/*.webp
 *   EON_EditeurSalles/Salles/              les .eonsalle (lus et écrits ici)
 * Unreal met ce dossier à jour (menu EON Salles > Publier la bibliothèque sur le Drive)
 * et importe les salles depuis Salles/. Ce fichier n'a pas à changer quand l'éditeur évolue.
 *
 * Pour viser un autre dossier : Paramètres du projet > Propriétés du script > DOSSIER_ID = <id du dossier>.
 */

var RACINE = "EON_EditeurSalles";

function racine_() {
  var id = PropertiesService.getScriptProperties().getProperty("DOSSIER_ID");
  if (id) return DriveApp.getFolderById(id);
  var it = DriveApp.getFoldersByName(RACINE);
  if (!it.hasNext()) throw new Error("Dossier « " + RACINE + " » introuvable sur le Drive.");
  return it.next();
}

function sousDossier_(nom, creer) {
  var r = racine_(), it = r.getFoldersByName(nom);
  if (it.hasNext()) return it.next();
  if (creer) return r.createFolder(nom);
  throw new Error("Dossier « " + RACINE + "/" + nom + " » introuvable sur le Drive.");
}

function doGet() {
  var it = racine_().getFilesByName("EditeurSalles.html");
  if (!it.hasNext()) throw new Error("EditeurSalles.html absent du dossier « " + RACINE + " ».");
  return HtmlService.createHtmlOutput(it.next().getBlob().getDataAsString("UTF-8"))
    .setTitle("EON — Éditeur de salles")
    .addMetaTag("viewport", "width=device-width, initial-scale=1");
}

// ------------------------------------------------------------------ bibliothèque d'assets

/** Liste des fichiers de la bibliothèque : [{id, path, t (date de modif en ms), n (octets)}]. */
function manifeste() {
  var out = [];
  (function walk(dossier, prefixe) {
    var fs = dossier.getFiles();
    while (fs.hasNext()) {
      var f = fs.next(), nom = f.getName();
      if (/\.(eonasset|eonapercu|eontheme|webp|png)$/i.test(nom))
        out.push({ id: f.getId(), path: prefixe + nom, t: f.getLastUpdated().getTime(), n: f.getSize() });
    }
    var ds = dossier.getFolders();
    while (ds.hasNext()) { var d = ds.next(); walk(d, prefixe + d.getName() + "/"); }
  })(sousDossier_("Bibliotheque", false), "");
  return out;
}

/** Contenu de fichiers par id : texte pour les fichiers JSON, base64 pour les images. */
function fichiers(ids) {
  return ids.map(function (id) {
    var f = DriveApp.getFileById(id), b = f.getBlob();
    if (/\.(webp|png)$/i.test(f.getName())) return { id: id, b64: Utilities.base64Encode(b.getBytes()), type: b.getContentType() };
    return { id: id, txt: b.getDataAsString("UTF-8") };
  });
}

// ------------------------------------------------------------------ salles

/** Toutes les salles : [{id, name, t, txt}] (quelques Ko chacune). */
function listeSalles() {
  var out = [], fs = sousDossier_("Salles", true).getFiles();
  while (fs.hasNext()) {
    var f = fs.next();
    if (!/\.eonsalle$/i.test(f.getName())) continue;
    out.push({ id: f.getId(), name: f.getName(), t: f.getLastUpdated().getTime(), txt: f.getBlob().getDataAsString("UTF-8") });
  }
  return out;
}

/**
 * Écrit une salle dans Salles/.
 * t : date de modification connue du fichier (null pour une nouvelle salle). Si le fichier
 * a changé depuis (autre PC) ou existe déjà sans qu'on le sache, rien n'est écrit et la
 * réponse dit {conflit: true} ; l'éditeur demande alors confirmation et renvoie force = true.
 */
function ecrireSalle(nom, txt, t, force) {
  if (!/^[^\\\/:*?"<>|]+\.eonsalle$/i.test(nom)) throw new Error("Nom de fichier invalide : " + nom);
  var dossier = sousDossier_("Salles", true), it = dossier.getFilesByName(nom), f = it.hasNext() ? it.next() : null;
  if (f && !force) {
    var tf = f.getLastUpdated().getTime();
    if (t == null || tf > t + 1000) return { conflit: true, t: tf };
  }
  if (f) f.setContent(txt);
  else f = dossier.createFile(nom, txt, "application/json");
  return { id: f.getId(), name: nom, t: f.getLastUpdated().getTime() };
}
