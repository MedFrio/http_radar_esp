#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ============================================
// CONFIGURATION WIFI
// ============================================
const char* WIFI_SSID = "Xiaomi_13T_Pro";
const char* WIFI_PASSWORD = "momo3521";

// ============================================
// CONFIGURATION DES PINS
// ============================================
const int TRIG_PIN = 5;   // Pin qui envoie le signal ultrason
const int ECHO_PIN = 18;  // Pin qui reçoit l'écho

// ============================================
// SERVEUR WEB
// ============================================
WebServer server(80);

// ============================================
// FONCTION: Mesurer la distance avec HC-SR04
// Retourne: distance en centimètres
// ============================================
float mesureDistance() {
  // 1. Envoyer une impulsion ultrasonore de 10µs
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // 2. Mesurer le temps que met l'écho à revenir (max 30ms)
  unsigned long duree = pulseIn(ECHO_PIN, HIGH, 30000);
  
  // 3. Si pas d'écho reçu (timeout), retourner une valeur invalide
  if (duree == 0) {
    return -1.0;
  }
  
  // 4. Calculer la distance
  // Vitesse du son = 343 m/s = 0.0343 cm/µs
  // Distance = (durée × vitesse) / 2
  // On divise par 2 car le son fait l'aller-retour
  float distance = (duree * 0.0343) / 2.0;
  
  return distance;
}

// ============================================
// FONCTION: Faire une moyenne de 3 mesures
// Pour avoir un résultat plus stable
// ============================================
float mesureMoyenne() {
  float total = 0.0;
  int mesuresValides = 0;
  
  // Faire 3 mesures
  for (int i = 0; i < 3; i++) {
    float distance = mesureDistance();
    
    // Si la mesure est valide, l'ajouter
    if (distance > 0) {
      total += distance;
      mesuresValides++;
    }
    
    delay(10); // Petite pause entre les mesures
  }
  
  // Calculer la moyenne si on a au moins une mesure valide
  if (mesuresValides > 0) {
    return total / mesuresValides;
  }
  
  return -1.0; // Pas de mesure valide
}

// ============================================
// PAGE HTML: Interface web moderne
// ============================================
String pageHTML() {
  return R"HTML(
<!DOCTYPE html>
<html lang="fr">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 - Capteur Distance</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    
    body {
      font-family: 'Segoe UI', system-ui, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    
    .container {
      background: white;
      border-radius: 20px;
      padding: 40px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
    }
    
    h1 {
      color: #667eea;
      text-align: center;
      margin-bottom: 10px;
      font-size: 2em;
    }
    
    .subtitle {
      text-align: center;
      color: #666;
      margin-bottom: 30px;
      font-size: 0.95em;
    }
    
    .distance-display {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      padding: 40px;
      border-radius: 15px;
      text-align: center;
      margin-bottom: 20px;
      box-shadow: 0 10px 30px rgba(102, 126, 234, 0.3);
    }
    
    .distance-value {
      font-size: 4em;
      font-weight: bold;
      margin-bottom: 10px;
    }
    
    .distance-unit {
      font-size: 1.2em;
      opacity: 0.9;
    }
    
    .info-box {
      display: flex;
      justify-content: space-between;
      background: #f5f5f5;
      padding: 15px 20px;
      border-radius: 10px;
      margin-bottom: 15px;
    }
    
    .info-label {
      color: #666;
      font-size: 0.9em;
    }
    
    .info-value {
      font-weight: bold;
      color: #333;
    }
    
    .status-ok { color: #4caf50; }
    .status-error { color: #f44336; }
    
    .controls {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
      margin-top: 20px;
    }
    
    button {
      padding: 15px;
      border: none;
      border-radius: 10px;
      font-size: 1em;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      color: white;
    }
    
    .btn-slow {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    }
    
    .btn-fast {
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
    }
    
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(0,0,0,0.2);
    }
    
    button:active {
      transform: translateY(0);
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Capteur de Distance</h1>
    <p class="subtitle">HC-SR04 + ESP32</p>
    
    <div class="distance-display">
      <div class="distance-value" id="distance">--</div>
      <div class="distance-unit">centimètres</div>
    </div>
    
    <div class="info-box">
      <span class="info-label">État:</span>
      <span class="info-value" id="status">Chargement...</span>
    </div>
    
    <div class="info-box">
      <span class="info-label">Dernière mesure:</span>
      <span class="info-value" id="time">--</span>
    </div>
    
    <div class="controls">
      <button class="btn-slow" onclick="setRefresh(1000)">Lent (1 sec)</button>
      <button class="btn-fast" onclick="setRefresh(200)">Rapide (0.2 sec)</button>
    </div>
  </div>

  <script>
    let refreshInterval = 1000; // Intervalle de rafraîchissement en ms
    let timer = null;
    
    // Fonction pour récupérer et afficher la distance
    async function updateDistance() {
      try {
        const response = await fetch('/api/distance');
        const data = await response.json();
        
        // Afficher la distance
        if (data.ok) {
          document.getElementById('distance').textContent = data.distance_cm.toFixed(1);
          document.getElementById('status').textContent = '✓ Mesure OK';
          document.getElementById('status').className = 'info-value status-ok';
        } else {
          document.getElementById('distance').textContent = '--';
          document.getElementById('status').textContent = '✗ Hors portée';
          document.getElementById('status').className = 'info-value status-error';
        }
        
        // Afficher l'heure
        const now = new Date();
        document.getElementById('time').textContent = now.toLocaleTimeString('fr-FR');
        
      } catch (error) {
        document.getElementById('status').textContent = '✗ Erreur connexion';
        document.getElementById('status').className = 'info-value status-error';
        console.error('Erreur:', error);
      }
    }
    
    // Fonction pour changer la vitesse de rafraîchissement
    function setRefresh(interval) {
      refreshInterval = interval;
      
      // Arrêter l'ancien timer
      if (timer) {
        clearInterval(timer);
      }
      
      // Démarrer un nouveau timer
      timer = setInterval(updateDistance, refreshInterval);
      updateDistance(); // Mise à jour immédiate
    }
    
    // Démarrer automatiquement
    setRefresh(1000);
  </script>
</body>
</html>
)HTML";
}

// ============================================
// ROUTE: Page d'accueil
// ============================================
void routeAccueil() {
  server.send(200, "text/html; charset=utf-8", pageHTML());
}

// ============================================
// ROUTE: API pour récupérer la distance
// Retourne un JSON avec la distance mesurée
// ============================================
void routeDistance() {
  // Mesurer la distance (moyenne de 3 mesures)
  float distance = mesureMoyenne();
  
  // Créer la réponse JSON
  String json = "{";
  json += "\"ok\": ";
  json += (distance > 0) ? "true" : "false";
  json += ", \"distance_cm\": ";
  json += (distance > 0) ? String(distance, 2) : "null";
  json += "}";
  
  // Envoyer la réponse
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ============================================
// SETUP: Initialisation
// ============================================
void setup() {
  // Démarrer la communication série
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP32 + Capteur HC-SR04 ===\n");
  
  // Configurer les pins du capteur
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("✓ Pins configurés");
  
  // Connexion au WiFi
  Serial.printf("Connexion au WiFi: %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Attendre la connexion (max 20 secondes)
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 40) {
    delay(500);
    Serial.print(".");
    tentatives++;
  }
  Serial.println();
  
  // Vérifier si connecté
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✓ WiFi connecté!");
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("✗ WiFi non connecté (timeout)");
  }
  
  // Configurer les routes du serveur web
  server.on("/", HTTP_GET, routeAccueil);
  server.on("/api/distance", HTTP_GET, routeDistance);
  server.onNotFound([]() {
    server.send(404, "text/plain", "404 - Page non trouvée");
  });
  
  // Démarrer le serveur
  server.begin();
  Serial.println("✓ Serveur HTTP démarré sur le port 80\n");
  Serial.println("Ouvrez votre navigateur et allez à l'adresse IP affichée ci-dessus");
}

// ============================================
// LOOP: Boucle principale
// ============================================
void loop() {
  // Gérer les requêtes HTTP
  server.handleClient();
}