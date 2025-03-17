# AutoGeneratorControl
# Système de Contrôle Automatique de Générateur

Ce projet permet de contrôler automatiquement un générateur électrique lors de coupures de courant. Il utilise un Arduino pour détecter les coupures du réseau électrique, démarrer automatiquement le générateur, et basculer l'alimentation via un commutateur de transfert automatique (ATS).

## Fonctionnalités

- Détection automatique des coupures de courant
- Démarrage et arrêt automatiques du générateur
- Contrôle du starter et du choke pour un démarrage fiable
- Surveillance de la tension du générateur avant la commutation
- Tentatives multiples de démarrage en cas d'échec
- Période d'attente avant démarrage pour éviter les réactions aux coupures brèves
- Période de refroidissement du générateur avant l'arrêt complet
- Journal d'événements pour le suivi et le diagnostic
- Protection contre les arrêts inattendus

## Matériel Requis

- Arduino (Uno, Mega, Nano, etc.)
- 4 modules relais pour:
  - Mise sous tension du générateur
  - Contrôle du choke
  - Contrôle du démarreur
  - Commutation ATS
- Capteurs de détection pour:
  - Présence du réseau électrique
  - Fonctionnement du générateur
- Diviseurs de tension pour mesurer:
  - Tension du réseau
  - Tension du générateur
- Fils de connexion
- Alimentation de secours pour l'Arduino

## Connexions des Broches

| Composant | Broche Arduino |
|-----------|----------------|
| Relais d'alimentation | 7 |
| Relais du choke | 6 |
| Relais du démarreur | 5 |
| Relais ATS | 4 |
| Détection du générateur (sortie) | 12 |
| Détection du générateur (entrée) | 13 |
| Détection du réseau (sortie) | 11 |
| Détection du réseau (entrée) | 10 |
| Mesure tension générateur | A0 |
| Mesure tension réseau | A1 |

## Temporisations du Système

| Événement | Durée |
|-----------|-------|
| Attente après perte du réseau | 5 minutes |
| Durée d'activation du choke | 3,5 secondes |
| Délai avant activation ATS | 2 minutes |
| Durée maximale de fonctionnement | 4 heures |
| Attente après retour du réseau | 2 minutes |
| Période de refroidissement | 2 minutes |

## Machine à États

Le système fonctionne selon une machine à états avec les étapes suivantes:

1. **IDLE**: Fonctionnement normal sur le réseau
2. **GRID_LOSS_DETECTED**: Perte de réseau détectée, attente avant démarrage
3. **START_POWER_ON**: Mise sous tension du générateur
4. **START_CHOKE_ON**: Activation du choke
5. **START_CRANKING**: Activation du démarreur
6. **START_CHOKE_OFF**: Désactivation du choke
7. **CHECK_RUNNING**: Vérification du démarrage
8. **RUNNING_WAIT_ATS**: Attente avant activation ATS
9. **RUNNING_WITH_ATS**: Fonctionnement sur générateur
10. **GRID_RESTORED_WAIT**: Réseau rétabli, attente avant commutation
11. **COOLING_DOWN**: Période de refroidissement du générateur
12. **SHUTTING_DOWN**: Arrêt du générateur

## Installation

1. Connectez les relais aux broches de sortie de l'Arduino comme indiqué dans le tableau des connexions
2. Connectez les circuits de détection pour le réseau et le générateur
3. Installez les diviseurs de tension pour mesurer les tensions
4. Téléversez le code dans l'Arduino
5. Calibrez le facteur de tension (`voltageFactor`) selon votre diviseur de tension

## Calibration

Le système nécessite une calibration du facteur de tension (`voltageFactor`) pour mesurer correctement les tensions. Pour calibrer:

1. Mesurez la tension réelle avec un multimètre
2. Notez la valeur lue sur l'entrée analogique (0-1023)
3. Calculez le facteur: `voltageFactor = tension_réelle / valeur_analogique`
4. Mettez à jour la valeur dans le code

## Personnalisation

Vous pouvez ajuster les temporisations dans le code selon vos besoins:
- `chokeOnTime`: Durée d'activation du choke
- `atsDelayTime`: Délai avant activation de l'ATS
- `generatorRunTime`: Durée maximale de fonctionnement
- `shutdownDelayTime`: Délai avant arrêt
- `gridLossWaitTime`: Temps d'attente après perte du réseau

## Dépannage

| Problème | Cause Possible | Solution |
|----------|----------------|----------|
| Le générateur ne démarre pas | Connexions des relais | Vérifiez les connexions des relais |
| | Batterie faible | Vérifiez la batterie du générateur |
| | Problème de choke | Ajustez la durée du choke |
| Commutation ATS ne se fait pas | Tension trop faible | Ajustez le seuil de tension minimal |
| | Connexion du relais ATS | Vérifiez les connexions du relais ATS |
| Fausses détections | Problème de debounce | Augmentez le délai de debounce |
| | Interférences | Vérifiez les connexions des capteurs |

## Sécurité

⚠️ **ATTENTION** ⚠️

- Ce système travaille avec des tensions dangereuses
- L'installation doit être réalisée par un électricien qualifié
- Assurez-vous que le générateur est installé dans un espace bien ventilé
- Respectez les normes électriques locales
- Testez régulièrement le système pour assurer son bon fonctionnement

## Extensions Possibles

- Interface utilisateur avec écran LCD
- Contrôle et surveillance à distance
- Enregistrement des données sur carte SD
- Surveillance de la consommation de carburant
- Intégration avec d'autres systèmes domotiques

---

Développé par [Votre Nom] - [Date]

Pour toute question ou suggestion, contactez [Votre Email]
