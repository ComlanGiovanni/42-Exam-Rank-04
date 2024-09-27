#!/bin/bash

# test_microshell.sh
# Script pour tester le programme microshell avec différents cas de test

MICROSHELL_EXEC="./microshell"

if [ ! -x "$MICROSHELL_EXEC" ]; then
  echo "Erreur : Le programme microshell n'est pas exécutable."
  echo "Veuillez compiler votre programme et vous assurer qu'il est exécutable."
  exit 1
fi

echo "Démarrage des tests pour le microshell..."

# Fonction pour exécuter un test
run_test() {
  TEST_NUM="$1"
  COMMAND="$2"
  EXPECTED_OUTPUT="$3"
  DESCRIPTION="$4"

  echo
  echo "=== Test $TEST_NUM ==="
  echo "Description : $DESCRIPTION"
  echo "Commande : $COMMAND"
  echo "Sortie attendue :"
  echo -e "$EXPECTED_OUTPUT"
  echo "--------------------------"
  echo "Sortie réelle :"

  # Exécuter la commande et capturer la sortie
  ACTUAL_OUTPUT=$(eval "$COMMAND" 2>&1)

  echo -e "$ACTUAL_OUTPUT"

  # Comparer la sortie réelle avec la sortie attendue
  if [ "$ACTUAL_OUTPUT" == "$EXPECTED_OUTPUT" ]; then
    echo "Résultat : Succès ✅"
  else
    echo "Résultat : Échec ❌"
  fi
}

# Test 1: Commande simple
run_test 1 \
"$MICROSHELL_EXEC /bin/echo 'Hello, World!'" \
"Hello, World!" \
"Test de l'exécution d'une commande simple avec des arguments."

# Test 2: Commande sans arguments
run_test 2 \
"$MICROSHELL_EXEC /bin/pwd" \
"$(pwd)" \
"Test d'une commande sans arguments."

# Test 3: Commande avec plusieurs arguments
run_test 3 \
"$MICROSHELL_EXEC /bin/echo 'Argument1' 'Argument2' 'Argument3'" \
"Argument1 Argument2 Argument3" \
"Test de l'exécution d'une commande avec plusieurs arguments."

# Test 4: Deux commandes séparées par un point-virgule
run_test 4 \
"$MICROSHELL_EXEC /bin/echo 'First Command' ';' /bin/echo 'Second Command'" \
$'First Command\nSecond Command' \
"Test de l'exécution séquentielle de commandes avec ';'."

# Test 5: Multiples points-virgules entre les commandes
run_test 5 \
"$MICROSHELL_EXEC /bin/echo 'First' ';' ';' ';' /bin/echo 'Second'" \
$'First\nSecond' \
"Vérification que le shell ignore les multiples ';'."

# Test 6: Point-virgule à la fin
run_test 6 \
"$MICROSHELL_EXEC /bin/echo 'Hello' ';'" \
"Hello" \
"Assurer qu'un ';' à la fin n'entraîne pas de problèmes."

# Test 7: Pipe simple entre deux commandes
run_test 7 \
"$MICROSHELL_EXEC /bin/echo -e 'Hello\nWorld' '|' /usr/bin/grep 'World'" \
"World" \
"Test du piping entre deux commandes."

# Test 8: Multiples pipes
run_test 8 \
"$MICROSHELL_EXEC /bin/echo -e 'Line1\nLine2\nLine3' '|' /usr/bin/grep 'Line' '|' /usr/bin/wc '-l'" \
"3" \
"Chaînage de plusieurs commandes avec des pipes."

# Test 9: Piping avec des commandes sans sortie
run_test 9 \
"$MICROSHELL_EXEC /bin/true '|' /bin/echo 'This should not appear'" \
"This should not appear" \
"Vérification du comportement avec des commandes sans sortie."

# Test 10: Piping dans 'cat'
run_test 10 \
"$MICROSHELL_EXEC /bin/echo 'Test' '|' /bin/cat '-' ';' /bin/echo 'Done'" \
$'Test\nDone' \
"Test de lecture depu
