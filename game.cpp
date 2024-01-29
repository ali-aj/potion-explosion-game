#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <unordered_map>
#include <semaphore.h>
#include <pthread.h>
#include <cstdlib>
#include <limits>
#include <cstdio> // for getchar()
using namespace std;

const int sharedMemSize = 1024; // Size of shared memory segment
const char* sharedMemName = "potion_explosion";
sem_t semaphore;
void* sharedMem; // Global variable for shared memory

const int totalBalls = 80;
const int totalColors = 4;

string dispenser[totalBalls / totalColors][totalColors];
string ballColors[totalColors] = { "Red", "Yellow", "Blue", "Black" };


struct Potion {
	string name;
	vector<string> ingredients;
	int score;
};

struct Player {
	string name;
	int score;
	vector<string> availableBalls;
	vector<Potion> potions;
	vector<Potion> completedPotions;
};

struct PlayerArgs {
	Player* currentPlayer;
	Player* otherPlayer;
};

// Define available potions
vector<Potion> potions = {
	Potion{ "Fireball", {"Red", "Yellow"}, 10 },
	Potion{ "Ice Blast", {"Blue", "Black"}, 15 },
	Potion{ "Abyssal Draft", {"Red", "Yellow", "Blue", "Black"}, 8 },
	Potion{ "Magnetic Attraction", {"Black", "Black"}, 6 },
	Potion{ "Prismatic Joy", {"Yellow", "Blue", "Red", "Red"}, 3 }
};


void removeExistingSharedMemory() {
	shm_unlink(sharedMemName);
}

void createSharedMemory() {
	int shm_fd = shm_open(sharedMemName, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (shm_fd == -1) {
		perror("shm_open");
		exit(EXIT_FAILURE);
	}

	ftruncate(shm_fd, sharedMemSize);

	sharedMem = mmap(NULL, sharedMemSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (sharedMem == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	close(shm_fd);
}

void updateSharedMem(const string& message) {
	// Update shared memory with the message
	strcpy((char*)sharedMem, message.c_str());
}

string readSharedMem() {
	// Read message from shared memory
	return string((char*)sharedMem);
}

void waitForEnter() {
	cout << "\tPress Enter to continue...";
	getchar();
	getchar();
}

void playerTurnMessage(const string& name) {
	string sharedMemData = readSharedMem();
	std::cout << "\n\n\t**************\n";
	std::cout << "\t*                                      *\n";
	std::cout << "\t*   " << setw(35) << left << sharedMemData << "*\n";
	std::cout << "\t*   It's your turn, " << setw(19) << left << name << "*\n";
	std::cout << "\t*                                      *\n";
	std::cout << "\t**************\n\n";
}

void displayPlayerScores(string player1Name, string player2Name, int scorePlayer1, int scorePlayer2) {
	std::cout << "\n\n\t**************\n";
	std::cout << "\t*                                      *\n";
	std::cout << "\t*   " << player1Name << "'s score: " << setw(19) << left << scorePlayer1 << "*\n";
	std::cout << "\t*   " << player2Name << "'s score: " << setw(19) << left << scorePlayer2 << "*\n";
	std::cout << "\t*                                      *\n";
	std::cout << "\t**************\n\n\n";
}

// Function to initialize a player
Player initializePlayer(int playerNumber) {
	Player player;
	cout << "\tEnter Player " << playerNumber << "'s name: ";
	getline(cin, player.name);
	player.score = 0;
	player.potions.resize(0);
	return player;
}

// Function to initialize the dispenser with random balls
void initializeDispenser() {
	int colorCounts[totalColors] = { 0 };

	// Seed the random number generator with the current time
	std::srand(static_cast<unsigned>(std::time(0)));

	for (int i = 0; i < totalBalls / totalColors; i++) {
		for (int j = 0; j < totalColors; j++) {
			int randomNumber;
			do {
				randomNumber = std::rand() % totalColors;
			} while (colorCounts[randomNumber] >= totalBalls / totalColors);

			dispenser[i][j] = ballColors[randomNumber];
			colorCounts[randomNumber]++;
		}
	}
}

int findPotionIndex(string potionName) {
	int index = 0;
	for (const Potion& potion : potions) {
		if (potion.name == potionName) {
			return index;
		}
		index++;
	}
	return -1;
}

bool isPotionComplete(const Potion& potion) {
	int index = findPotionIndex(potion.name);

	if (index != -1) {
		const Potion& targetPotion = potions[index];

		// Count occurrences of each color in potions[index]
		unordered_map<string, int> targetColorCounts;
		for (const string& color : targetPotion.ingredients) {
			targetColorCounts[color]++;
		}

		// Count occurrences of each color in potion
		unordered_map<string, int> potionColorCounts;
		for (const string& color : potion.ingredients) {
			potionColorCounts[color]++;
		}

		// Compare occurrences of each color
		for (const auto& entry : targetColorCounts) {
			const string& color = entry.first;
			int targetCount = entry.second;
			int potionCount = potionColorCounts[color];

			if (potionCount < targetCount) {
				return false;
			}
		}

		return true;
	}

	return false; // Potion not found in the potions vector
}

void displayDispenser(Player& player, string otherPlayerName, int otherPlayerScore) {
	playerTurnMessage(player.name);
	displayPlayerScores(player.name, otherPlayerName, player.score, otherPlayerScore);
	cout << "\n\t\t\tDispenser\n";
	cout << "\t\t\t----------\n";

	// Display column headers
	cout << "      ";
	for (int i = 0; i < totalColors; i++) {
		cout << setw(11) << "Column " << (i + 1);
	}
	cout << "\n";

	// Display dispenser content
	for (int i = 0; i < totalBalls / totalColors; i++) {
		cout << "\t" << setw(2) << i + 1 << ". ";
		for (int j = 0; j < totalColors; j++) {
			cout << setw(10) << dispenser[i][j];
		}
		cout << "\n";
	}
}

// Function to display the current selected potion tiles and completed potion tiles for a player
void displayPotionTiles(const Player& player) {
	// Display current selected potion tiles
	cout << "\n\tCurrent Potion Tiles:\n";
	int count = 1;
	for (const Potion& potion : player.potions) {
		cout << "\t" << count++ << ". " << potion.name << " (Score: " << potion.score << ")\n";
		int ingredientCount = 0;
		cout << "\t   Available => { ";
		for (const string& ingredient : potion.ingredients) {
			cout << ingredient;
			if (++ingredientCount < potion.ingredients.size()) {
				cout << ", ";
			}
		}
		cout << " }\n";

		cout << "\t   Required => { ";
		int index = findPotionIndex(potion.name);
		ingredientCount = 0;
		for (const string& ingredient : potions[index].ingredients) {
			cout << ingredient;
			if (++ingredientCount < potions[index].ingredients.size()) {
				cout << ", ";
			}
		}
		cout << " }\n";
	}
	cout << "\n\n";

	// Display completed potion tiles
	if (player.completedPotions.size() != 0) {
		cout << "\n\tCompleted Potion Tiles : \n";
		count = 1;
		for (const Potion& completedPotion : player.completedPotions) {
			cout << "\t" << count++ << ". " << completedPotion.name << " (Score: " << completedPotion.score << ")\n";
			cout << "\t=> { ";
			int ingredientCount = 0;
			for (const string& ingredient : completedPotion.ingredients) {
				cout << ingredient;
				if (++ingredientCount < completedPotion.ingredients.size()) {
					cout << ", ";
				}
			}
			cout << " }\n";
		}
		cout << endl;
	}
}

void displayAvailableBalls(const Player& player) {
	if (!player.availableBalls.empty()) {
		cout << "\tAvailable Balls: { ";
		for (size_t i = 0; i < player.availableBalls.size(); i++) {
			cout << player.availableBalls[i];
			if (i != player.availableBalls.size() - 1) {
				cout << ", ";
			}
		}
		cout << " }" << endl;
	}
	
}

void shiftBallsDown(Player& player, int row, int column) {
	// Shift the balls down
	int rowCopy = row;
	for (int i = rowCopy - 1; i >= 0; i--) {
		if (dispenser[i][column] != "-") {
			swap(dispenser[i][column], dispenser[row][column]);
			row--;
		}
	}
}

void addAvailableBalls(Player& player, int row, int column) {
	// Remove the selected ball from the dispenser
	player.availableBalls.push_back(dispenser[row][column]);
	dispenser[row][column] = "-";

	shiftBallsDown(player, row, column);

	// Check for explosions
	while (row < (totalBalls / totalColors - 1) && dispenser[row][column] == dispenser[row + 1][column]) {

		string ballColor = dispenser[row][column];
		int up = row, down = row + 1;

		while (up >= 0) {
			if (dispenser[up][column] == ballColor) {
				player.availableBalls.push_back(ballColor);
				dispenser[up][column] = "-";
			}
			else {
				break;
			}
			up--;
		}

		while (down < (totalBalls / totalColors)) {
			if (dispenser[down][column] == ballColor) {
				player.availableBalls.push_back(ballColor);
				dispenser[down][column] = "-";
				row = down;
			}
			else {
				break;
			}
			down++;
		}
		shiftBallsDown(player, row, column);
	}
}

// Function to print colored text
void printInColor(const std::string& text, const std::string& color) {
	std::cout << "\033[" << color << "m" << text << "\033[0m";
}

void displayWelcomeMessage() {
	system("clear"); // Clear the console screen

	// Print welcome message in color
	printInColor("\n\n\t\t\t\t\t\t\t\t\t\t\tWelcome to Potion Explosion!\n\n\n\n\n", "1;32");

	std::cout << "\tPress Enter to start...\n\n\t";
	std::cin.ignore(); // Wait for user input (Enter)
}

void choosePotionTiles(Player& player, string otherPlayerName, int otherPlayerScore) {
	int i = 1;
	playerTurnMessage(player.name);
	displayPlayerScores(player.name, otherPlayerName, player.score, otherPlayerScore);
	cout << "\n\n\tAvailable Potion Tiles:\n";
	for (const Potion& potion : potions) {
		cout << "\t" << i++ << ". " << potion.name << " (Score: " << potion.score << ")\n\t=> { ";
		int ingredientCount = 0;
		for (const string& ingredient : potion.ingredients) {
			cout << ingredient;
			if (++ingredientCount < potion.ingredients.size()) {
				cout << ", ";
			}
		}
		cout << " }\n\n";
	}

	if (player.potions.size() == 0) {
		int potion1, potion2;
		cout << "\tChoose tiles by entering their numbers (e.g., 1 3 OR 1 4): ";
		cin >> potion1 >> potion2;

		player.potions.push_back(Potion{ potions[potion1 - 1].name , {}, potions[potion1 - 1].score });
		player.potions.push_back(Potion{ potions[potion2 - 1].name , {}, potions[potion2 - 1].score });
	}
	else {
		int potion1;
		cout << "\tChoose a tile by entering their number: ";
		cin >> potion1;

		player.potions.push_back(Potion{ potions[potion1 - 1].name , {}, potions[potion1 - 1].score });
	}
}

void pickABall(Player& player) {
	// Player selects a ball from the dispenser
	cout << "\t" << "Select a ball from the dispenser (enter row and column e.g: 1 2): ";
	int selectedRow, selectedColumn;
	do {
		cin >> selectedRow >> selectedColumn;
		if ((selectedRow > totalBalls / totalColors || selectedRow < 1) && (selectedColumn > totalColors || selectedColumn < 1)) {
			cout << "\tInvalid Input. Enter again.\n\t";
		}
	} while ((selectedRow > totalBalls / totalColors || selectedRow < 1) && (selectedColumn > totalColors || selectedColumn < 1));

	// Update the dispenser after the player picks a ball and also checking for explosions
	addAvailableBalls(player, selectedRow - 1, selectedColumn - 1);
}

bool isPotionTileSelectPossible(const Player& player, int tile) {
	int index = findPotionIndex(player.potions[tile].name);
	vector<string> requiredIngredients = potions[index].ingredients;

	for (const string& potionIngredient : player.potions[tile].ingredients) {
		auto it = find(requiredIngredients.begin(), requiredIngredients.end(), potionIngredient);
		if (it != requiredIngredients.end()) {
			requiredIngredients.erase(it);
		}
	}

	for (const string& reqIngredients : requiredIngredients) {
		for (const string availBalls : player.availableBalls) {
			if (reqIngredients == availBalls) {
				return true;
			}
		}
	}

	return false;
}

vector<string> RequiredBallsForPotionTile(const Player& player, int tile) {

	int index = findPotionIndex(player.potions[tile].name);
	vector<string> requiredIngredients = potions[index].ingredients;

	for (const string& potionIngredient : player.potions[tile].ingredients) {
		auto it = find(requiredIngredients.begin(), requiredIngredients.end(), potionIngredient);
		if (it != requiredIngredients.end()) {
			requiredIngredients.erase(it);
		}
	}
	return requiredIngredients;
}

void displayAvailableRequiredBallsForPotionTile(const vector<string>& requiredIngredients) {
	int count = 0;
	for (const string& reqIngredients : requiredIngredients) {
		cout << "\t" << count++ << ". " << reqIngredients << endl;
	}
	cout << "\t(-1. Return to previous step.)\n\t";
}

string selectBallForPotionTiles(const vector<string>& requiredIngredients) {
	int selectBall;
	do {
		cin >> selectBall;
		if (selectBall < -1 || selectBall >= requiredIngredients.size()) {
			cout << "\tInvalid input. Enter Again!\n\t";
		}
	} while (selectBall < -1 || selectBall >= requiredIngredients.size());
	if (selectBall != -1) {
		return requiredIngredients[selectBall];
	}
	return "-";
}

vector<string> availableRequiredBallsForPotionTiles(const vector<string> requiredBalls, const Player& player) {
	vector<string> availableRequiredBalls;
	vector<string> availableBallsForTiles = player.availableBalls;

	for (const string& reqBall : requiredBalls) {
		auto it = find(availableBallsForTiles.begin(), availableBallsForTiles.end(), reqBall);
		if (it != availableBallsForTiles.end()) {
			availableRequiredBalls.push_back(reqBall);
			availableBallsForTiles.erase(it);
		}
	}

	return availableRequiredBalls;
}

void brewPotions(Player& player, string otherPlayerName, int otherPlayerScore) {
	// Ask player to place the balls on Potion Tiles
	while (true) {
		if (player.potions.size() == 2) {
			if (!isPotionTileSelectPossible(player, 0) && !isPotionTileSelectPossible(player, 1)) {
				return;
			}
		}
		else if (!isPotionTileSelectPossible(player, 0)) {
			return;
		}
		int tile;
		do {
			cout << "\n\tPress the Potion tile number to select it: ";
			cin >> tile;
			if (tile != 1 && tile != 2) {
				cout << "\n\tInvalid input.Enter Again!\n\t";
			}
			else {
				if (!isPotionTileSelectPossible(player, tile - 1)) {
					cout << "\n\tNOTE: YOU CANNOT SELECT THIS POTION TILE SINCE YOU DONT HAVE REQUIRED BALL(s).\n";
				}
			}
		} while ((tile != 1 && tile != 2) || !isPotionTileSelectPossible(player, tile - 1));

		vector<string> requiredBalls = RequiredBallsForPotionTile(player, tile - 1);
		vector<string> availableRequiredBalls = availableRequiredBallsForPotionTiles(requiredBalls, player);
		while (true) {

			if (availableRequiredBalls.size() == 0) {
				break;
			}
			cout << "\n\tPress to select the ball:\n";
			displayAvailableRequiredBallsForPotionTile(availableRequiredBalls);

			string selectedBall = selectBallForPotionTiles(availableRequiredBalls);
			if (selectedBall == "-") {
				break;
			}

			player.potions[tile - 1].ingredients.push_back(selectedBall);
			player.availableBalls.erase(std::find(player.availableBalls.begin(), player.availableBalls.end(), selectedBall));
			availableRequiredBalls.erase(std::find(availableRequiredBalls.begin(), availableRequiredBalls.end(), selectedBall));

			if (isPotionComplete(player.potions[tile - 1])) {
				// Create a new Potion object with the same properties
				Potion completedPotion{
					player.potions[tile - 1].name,
					player.potions[tile - 1].ingredients,
					player.potions[tile - 1].score
				};

				// Add the completed potion to the player's completedPotions vector
				player.completedPotions.push_back(completedPotion);

				// Remove the completed potion from the player's potions vector
				auto it = std::find_if(player.potions.begin(), player.potions.end(),
					[&completedPotion](const Potion& potion) {
						return potion.name == completedPotion.name;
					});

				if (it != player.potions.end()) {
					player.potions.erase(it);
				}

				// Update the player's score
				player.score += completedPotion.score;

				// Display a success message
				cout << "\t" << player.name << " successfully brewed a " << completedPotion.name << " potion!\n";
				cout << "\t" << player.name << "'s Score: " << player.score << "\n";
			}

			waitForEnter();
			system("clear");
			displayDispenser(player, otherPlayerName, otherPlayerScore);
			displayPotionTiles(player);
			displayAvailableBalls(player);
		}

		
	}
}

bool isGameComplete(const Player& player1, const Player& player2) {
	for (const Potion& potion : potions) {
		// Check if the potion is present in player1's completed potions
		auto itPlayer1 = find_if(player1.completedPotions.begin(), player1.completedPotions.end(),
			[&potion](const Potion& completedPotion) {
				return completedPotion.name == potion.name;
			});

		// Check if the potion is present in player2's completed potions
		auto itPlayer2 = find_if(player2.completedPotions.begin(), player2.completedPotions.end(),
			[&potion](const Potion& completedPotion) {
				return completedPotion.name == potion.name;
			});

		// If the potion is not found in either player, the game is not complete
		if (itPlayer1 == player1.completedPotions.end() || itPlayer2 == player2.completedPotions.end()) {
			return false;
		}
	}
	// If all potions are found in both players, the game is complete
	return true;
}

void addRemainingBallsToDispenser(Player& player) {
	for (int i = (totalBalls / totalColors) - 1; i >= 0; i--) {
		for (int j = totalColors - 1; j >= 0; j--) {
			if (dispenser[i][j] == "-" && !player.availableBalls.empty()) {
				dispenser[i][j] = player.availableBalls.back();
				player.availableBalls.pop_back();
			}
		}
		if (player.availableBalls.empty()) {
			break;
		}
	}
}


void* player1Turn(void* arg) {
	PlayerArgs* args = (PlayerArgs*)arg;
	Player* player1 = args->currentPlayer;
	Player* player2 = args->otherPlayer;

	while (!isGameComplete(*player1, *player2)) {
		// Wait for turn
		sem_wait(&semaphore);


		if ((*player1).potions.size() < 2) {
			choosePotionTiles(*player1, player2->name, player2->score);
			waitForEnter();
			system("clear");
		}
		else {
			displayDispenser(*player1, player2->name, player2->score);
			displayPotionTiles(*player1);

			pickABall(*player1);

			waitForEnter();
			system("clear");

			displayDispenser(*player1, player2->name, player2->score);
			displayPotionTiles(*player1);
			displayAvailableBalls(*player1);

			brewPotions(*player1, player2->name, player2->score);

			addRemainingBallsToDispenser(*player1);

			waitForEnter();
			system("clear");
		}
		// Update shared memory with player's turn information
		updateSharedMem((*player1).name + " completed their turn.");


		sem_post(&semaphore);
		// Signal turn completion
		sleep(3); // Add a delay for better readability
	}
	return nullptr;
}

void* player2Turn(void* arg) {
	PlayerArgs* args = (PlayerArgs*)arg;
	Player* player2 = args->currentPlayer;
	Player* player1 = args->otherPlayer;

	while (!isGameComplete(*player1, *player2)) {
		// Wait for turn
		sem_wait(&semaphore);


		if ((*player2).potions.size() < 2) {
			choosePotionTiles(*player2, player1->name, player1->score);
			waitForEnter();
			system("clear");
		}
		else {
			displayDispenser(*player2, player1->name, player1->score);
			displayPotionTiles(*player2);

			pickABall(*player2);

			waitForEnter();
			system("clear");

			displayDispenser(*player2, player1->name, player1->score);
			displayPotionTiles(*player2);
			displayAvailableBalls(*player2);

			brewPotions(*player2, player1->name, player1->score);

			addRemainingBallsToDispenser(*player2);

			waitForEnter();
			system("clear");
		}

		// Update shared memory with player's turn information
		updateSharedMem((*player2).name + " completed their turn.");

		// Signal turn completion
		sem_post(&semaphore);
		sleep(3); // Add a delay for better readability
	}
	return nullptr;
}

int main() {
	displayWelcomeMessage();
	system("clear");
	initializeDispenser();

	srand(static_cast<unsigned>(time(nullptr)));

	removeExistingSharedMemory();
	createSharedMemory();

	// Initialize players
	Player player1 = initializePlayer(1);
	Player player2 = initializePlayer(2);
	system("clear");

	// Initialize semaphore
	sem_init(&semaphore, 1, 1);

	// Fork a new process for both players
	pid_t pid = fork();

	if (pid < 0) {
		cerr << "Fork failed!\n";
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		// Child process

		pthread_t playerOne, playerTwo;

		PlayerArgs args1 = { &player1, &player2 };
		pthread_create(&playerOne, NULL, &player1Turn, (void*)&args1);

		PlayerArgs args2 = { &player2, &player1 };
		pthread_create(&playerTwo, NULL, &player2Turn, (void*)&args2);

		// Wait for both threads to finish
		pthread_join(playerOne, NULL);
		pthread_join(playerTwo, NULL);
	}
	else {
		// Parent process

		// Wait for the child process to complete
		waitpid(pid, nullptr, 0);

		// Remove the semaphore
		sem_destroy(&semaphore);

		// Display final score and winner
		if (player1.score > player2.score) {
			cout << "\t" << player1.name << " wins! \n";
		}
		else if (player1.score < player2.score) {
			cout << "\t" << player2.name << " wins! \n";
		}
		else {
			cout << "\t" << "It's a tie! \n";
		}
	}

	return 0;
}