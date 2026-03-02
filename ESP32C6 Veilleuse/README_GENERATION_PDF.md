# Génération du Guide PDF

## Méthode 1: Script Python (Recommandé)

### Prérequis
```bash
# Installer Python 3.8+ si nécessaire
python3 --version

# Installer les dépendances
pip install markdown weasyprint
```

### Génération
```bash
# Exécuter le script
python generate_pdf_guide.py
```

Le PDF sera généré sous le nom `guide_installation_proxmox.pdf`

## Méthode 2: Pandoc (Alternative)

### Installation de Pandoc
```bash
# Sur Ubuntu/Debian
sudo apt-get install pandoc

# Sur CentOS/RHEL
sudo yum install pandoc

# Sur Windows (avec Chocolatey)
choco install pandoc

# Sur macOS (avec Homebrew)
brew install pandoc
```

### Génération
```bash
# Commande directe
pandoc proxmox_installation_guide.md -o guide_installation_proxmox.pdf --pdf-engine=weasyprint --toc

# Avec style CSS personnalisé
pandoc proxmox_installation_guide.md -o guide_installation_proxmox.pdf --css=style.css --toc --toc-depth=3
```

## Méthode 3: Outils en ligne

Si vous préférez ne pas installer de logiciel:

1. **Markdown to PDF Converter**: https://markdowntopdf.com/
2. **Online Markdown Editor**: https://dillinger.io/
3. **GitHub**: Télécharger directement depuis GitHub en format PDF

## Personnalisation du style

Le script Python inclut déjà un style CSS personnalisé avec:
- Police professionnelle (DejaVu Sans)
- Mise en page optimisée pour l'impression
- Coloration syntaxique pour le code
- Numérotation des pages
- Table des matières

Pour modifier le style, éditez la variable `css_style` dans le fichier `generate_pdf_guide.py`.

## Dépannage

### Problèmes courants
- **WeasyPrint non installé**: `pip install weasyprint`
- **Problèmes de polices**: Installer les polices DejaVu
- **Erreur de mémoire**: Réduire la taille du document ou augmenter la RAM

### Sur Windows
```powershell
# Installation avec pip
pip install markdown weasyprint

# Installation des dépendances Windows
pip install cairocffi
```

### Vérification
```bash
# Vérifier que le PDF a été généré
ls -la guide_installation_proxmox.pdf

# Vérifier le contenu
file guide_installation_proxmox.pdf
```
