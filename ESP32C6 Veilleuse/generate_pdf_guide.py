#!/usr/bin/env python3
"""
Script pour générer un PDF à partir du guide d'installation Proxmox
"""

import os
import sys
from pathlib import Path

def install_requirements():
    """Installation des dépendances nécessaires"""
    requirements = [
        "markdown",
        "weasyprint",
        "markdown-pdf",
        "mdpdf"
    ]
    
    print("Installation des dépendances...")
    for req in requirements:
        try:
            __import__(req.replace('-', '_'))
            print(f"✓ {req} déjà installé")
        except ImportError:
            print(f"Installation de {req}...")
            os.system(f"{sys.executable} -m pip install {req}")

def generate_pdf_with_weasyprint():
    """Générer le PDF en utilisant WeasyPrint"""
    try:
        import markdown
        from weasyprint import HTML, CSS
        
        # Lire le fichier markdown
        with open('proxmox_installation_guide.md', 'r', encoding='utf-8') as f:
            md_content = f.read()
        
        # Convertir markdown en HTML
        html_content = markdown.markdown(md_content, extensions=['codehilite', 'tables', 'toc'])
        
        # Ajouter le style CSS
        css_style = """
        <style>
        body {
            font-family: 'DejaVu Sans', Arial, sans-serif;
            line-height: 1.6;
            margin: 2cm;
            font-size: 12pt;
        }
        h1, h2, h3, h4, h5, h6 {
            color: #2c3e50;
            page-break-after: avoid;
        }
        h1 {
            font-size: 24pt;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }
        h2 {
            font-size: 18pt;
            border-bottom: 2px solid #3498db;
            padding-bottom: 5px;
        }
        h3 {
            font-size: 14pt;
        }
        code {
            background-color: #f8f9fa;
            padding: 2px 4px;
            border-radius: 3px;
            font-family: 'Courier New', monospace;
            font-size: 10pt;
        }
        pre {
            background-color: #f8f9fa;
            padding: 10px;
            border-radius: 5px;
            border-left: 4px solid #3498db;
            overflow-x: auto;
            page-break-inside: avoid;
        }
        pre code {
            background-color: transparent;
            padding: 0;
        }
        blockquote {
            border-left: 4px solid #3498db;
            margin-left: 0;
            padding-left: 20px;
            font-style: italic;
        }
        table {
            border-collapse: collapse;
            width: 100%;
            margin: 20px 0;
        }
        th, td {
            border: 1px solid #ddd;
            padding: 8px;
            text-align: left;
        }
        th {
            background-color: #f2f2f2;
            font-weight: bold;
        }
        .page-break {
            page-break-before: always;
        }
        @page {
            margin: 2cm;
            @bottom-right {
                content: counter(page);
                font-size: 10pt;
            }
        }
        </style>
        """
        
        # Créer le HTML complet
        full_html = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="utf-8">
            <title>Guide d'Installation Proxmox</title>
            {css_style}
        </head>
        <body>
            {html_content}
        </body>
        </html>
        """
        
        # Générer le PDF
        html_doc = HTML(string=full_html)
        html_doc.write_pdf('guide_installation_proxmox.pdf')
        
        print("✓ PDF généré avec succès : guide_installation_proxmox.pdf")
        return True
        
    except ImportError as e:
        print(f"Erreur: {e}")
        return False
    except Exception as e:
        print(f"Erreur lors de la génération du PDF: {e}")
        return False

def generate_pdf_with_pandoc():
    """Alternative: Générer le PDF en utilisant pandoc"""
    try:
        # Vérifier si pandoc est installé
        import subprocess
        result = subprocess.run(['pandoc', '--version'], capture_output=True, text=True)
        
        if result.returncode == 0:
            print("Génération du PDF avec pandoc...")
            cmd = [
                'pandoc',
                'proxmox_installation_guide.md',
                '-o', 'guide_installation_proxmox.pdf',
                '--pdf-engine=weasyprint',
                '--css=style.css',
                '--toc',
                '--toc-depth=3',
                '-V', 'geometry:margin=2cm',
                '-V', 'fontsize=12pt'
            ]
            
            # Créer un fichier CSS simple si nécessaire
            css_content = """
            body { font-family: Arial, sans-serif; line-height: 1.6; margin: 2cm; }
            h1, h2, h3 { color: #2c3e50; }
            code { background-color: #f8f9fa; padding: 2px 4px; border-radius: 3px; }
            pre { background-color: #f8f9fa; padding: 10px; border-radius: 5px; }
            """
            
            with open('style.css', 'w') as f:
                f.write(css_content)
            
            subprocess.run(cmd, check=True)
            os.remove('style.css')  # Nettoyer le fichier CSS temporaire
            
            print("✓ PDF généré avec succès : guide_installation_proxmox.pdf")
            return True
        else:
            print("Pandoc n'est pas installé")
            return False
            
    except FileNotFoundError:
        print("Pandoc n'est pas installé")
        return False
    except Exception as e:
        print(f"Erreur avec pandoc: {e}")
        return False

def main():
    """Fonction principale"""
    print("Génération du guide d'installation Proxmox en PDF")
    print("=" * 50)
    
    # Vérifier si le fichier markdown existe
    if not os.path.exists('proxmox_installation_guide.md'):
        print("Erreur: le fichier 'proxmox_installation_guide.md' n'existe pas")
        return
    
    # Installer les dépendances
    install_requirements()
    
    # Essayer de générer le PDF avec WeasyPrint
    print("\nTentative de génération avec WeasyPrint...")
    if generate_pdf_with_weasyprint():
        print("\n✓ PDF généré avec succès!")
        print("Fichier: guide_installation_proxmox.pdf")
    else:
        print("\n✗ Échec de la génération avec WeasyPrint")
        print("Tentative avec pandoc...")
        if generate_pdf_with_pandoc():
            print("\n✓ PDF généré avec succès!")
            print("Fichier: guide_installation_proxmox.pdf")
        else:
            print("\n✗ Échec de la génération du PDF")
            print("Veuillez installer WeasyPrint ou Pandoc manuellement:")
            print("  pip install weasyprint markdown")
            print("  ou")
            print("  sudo apt-get install pandoc")

if __name__ == "__main__":
    main()
