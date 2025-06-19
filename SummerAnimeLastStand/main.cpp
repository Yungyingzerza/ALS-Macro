#include "GameManager.h"

int main()
{

    GameManager game;
    cv::Mat frame;

    //-=-=-=-=-EDIT BELOW THIS-=-=-=-=-

	cv::Rect towersRegion(1800, 200, 650, 200);
	cv::Rect unitManagerRegion(1710, 240, 840, 900);
    cv::Rect retryRegion(800, 850, 950, 150);// 1735, 989
	cv::Rect startRegion(950, 1050, 350, 150); //1278, 1150
	cv::Rect bossRegion(1234, 213, 100, 50); //1332, 258
    cv::Mat phanse1Image = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\phase1.png", cv::IMREAD_COLOR);
    cv::Mat phanse11Image = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\phase11.png", cv::IMREAD_COLOR);
    cv::Mat retryImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\retry.png", cv::IMREAD_COLOR);
    cv::Mat startImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\start.png", cv::IMREAD_COLOR);

	cv::Mat idolImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\idol.png", cv::IMREAD_COLOR);
	cv::Mat bulmyImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\bulmy.png", cv::IMREAD_COLOR);
	cv::Mat yunoImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\yuno.png", cv::IMREAD_COLOR);
	cv::Mat ichikoImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\ichiko.png", cv::IMREAD_COLOR);
	cv::Mat gilgameshImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\gilgamesh.png", cv::IMREAD_COLOR);

	cv::Mat bossImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\boss.png", cv::IMREAD_COLOR);


    // Check if the image was successfully loaded
    if (phanse1Image.empty() || phanse11Image.empty() || retryImage.empty() || startImage.empty()) {
        std::cerr << "Error: Could not load Image." << std::endl;
        return -1;
    }

	// Check if the image was successfully loaded
	if (idolImage.empty() || bulmyImage.empty() || yunoImage.empty() || ichikoImage.empty() || gilgameshImage.empty()) {
		std::cerr << "Error: Could not load Image." << std::endl;
		return -1;
	}

    //-=-=-=-=-EDIT ABOVE THIS-=-=-=-=-=-

		//-=-=-=-=-EDIT BELOW THIS-=-=-=-=-

		bool isPhase1Success = false;
		bool isUpgrade = false;
		bool pause = false;


		while (true) {

			//handle pause
			bool wasPressed = false;
			if ((GetAsyncKeyState('P') & 0x8000) && !wasPressed) {
				pause = !pause;  // toggle pause
				std::cout << (pause ? "[Paused]\n" : "[Resumed]\n");
				wasPressed = true;
			}
			else if (!(GetAsyncKeyState('P') & 0x8000)) {
				wasPressed = false; // reset when key released
			}

			if (GetAsyncKeyState('Z') & 0x8000) {
				break;
			}

			while (pause) {
				Sleep(100);
			}

			game.CaptureDXGI(frame);

			if (frame.empty()) break;

			cv::Mat units = frame(unitManagerRegion);
			cv::Mat startFrame = frame(startRegion);
			cv::Mat retryFrame = frame(retryRegion);
			cv::Mat bossFrame = frame(bossRegion);

			if (units.empty()) break;
			if (startFrame.empty()) break;
			if (retryFrame.empty()) break;
			if (bossFrame.empty()) break;

			cv::cvtColor(units, units, cv::COLOR_BGRA2BGR);
			cv::cvtColor(startFrame, startFrame, cv::COLOR_BGRA2BGR);
			cv::cvtColor(retryFrame, retryFrame, cv::COLOR_BGRA2BGR);
			cv::cvtColor(bossFrame, bossFrame, cv::COLOR_BGRA2BGR);

			cv::Mat yuno, idol, bulmy, ichiko, gilgamesh;
			cv::matchTemplate(units, yunoImage, yuno, cv::TM_CCOEFF_NORMED);
			cv::matchTemplate(units, idolImage, idol, cv::TM_CCOEFF_NORMED);
			cv::matchTemplate(units, bulmyImage, bulmy, cv::TM_CCOEFF_NORMED);
			cv::matchTemplate(units, ichikoImage, ichiko, cv::TM_CCOEFF_NORMED);
			cv::matchTemplate(units, gilgameshImage, gilgamesh, cv::TM_CCOEFF_NORMED);

			if (yuno.empty() || idol.empty() || bulmy.empty() || ichiko.empty() || gilgamesh.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			cv::Mat start;
			cv::matchTemplate(startFrame, startImage, start, cv::TM_CCOEFF_NORMED);

			if (start.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			cv::Mat retry;
			cv::matchTemplate(retryFrame, retryImage, retry, cv::TM_CCOEFF_NORMED);

			if (retry.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			cv::Mat boss;
			cv::matchTemplate(bossFrame, bossImage, boss, cv::TM_CCOEFF_NORMED);
			if (boss.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			double minValYuno, maxValYuno, minValIdol, maxValIdol, minValBulmy, maxValBulmy, minValIchiko, maxValIchiko, minValGilgamesh, maxValGilgamesh;
			cv::minMaxLoc(yuno, &minValYuno, &maxValYuno);
			cv::minMaxLoc(idol, &minValIdol, &maxValIdol);
			cv::minMaxLoc(bulmy, &minValBulmy, &maxValBulmy);
			cv::minMaxLoc(ichiko, &minValIchiko, &maxValIchiko);
			cv::minMaxLoc(gilgamesh, &minValGilgamesh, &maxValGilgamesh);

			double minValStart, maxValStart;
			cv::minMaxLoc(start, &minValStart, &maxValStart);

			double minValRetry, maxValRetry;
			cv::minMaxLoc(retry, &minValRetry, &maxValRetry);

			double minValBoss, maxValBoss;
			cv::minMaxLoc(boss, &minValBoss, &maxValBoss);

			std::cout << "--Unit Values--\n";
			std::cout << "Idol Val: " << maxValIdol << '\n';
			std::cout << "Bulmy Val: " << maxValBulmy << '\n';
			std::cout << "Ichiko Val: " << maxValIchiko << '\n';
			std::cout << "Yuno Val: " << maxValYuno << '\n';
			std::cout << "Gilgamesh Val: " << maxValGilgamesh << '\n';
			std::cout << "Start Val: " << maxValStart << '\n';
			std::cout << "Retry Val: " << maxValRetry << '\n';
			std::cout << "Boss Val: " << maxValBoss << '\n';
			std::cout << "----------------\n";

			//start
			if(maxValStart > 0.5) {
				Sleep(200);

				game.mouseScroll(true);
				game.panCameraRight(200);

				Sleep(2000);

				//open unit manager
				game.mouseMove(2388, 769);
				game.leftClick();

				Sleep(200);

				//start
				game.mouseMove(1143, 1115);
				game.leftClick();

				//reset all bool values
				isPhase1Success = false;
				isUpgrade = false;

			}

			//phase1 try to place idol, bulmy
			//if found idol and bulmy then set phase1Success to true
			if (maxValIdol > 0.7 && maxValBulmy > 0.7 && !isPhase1Success) {
				isPhase1Success = true;
			}
			else if(!isPhase1Success) {
				//place idol
				game.pressKey('4');
				game.mouseMove(1054, 646);
				game.leftClick();

				//place bulmy
				game.pressKey('6');
				game.mouseMove(1650, 838);
				game.leftClick();
			}

			//phase2 try to place yuno, ichiko, gilgamesh and phase1Success
			if ((maxValYuno < 0.7 || maxValIchiko < 0.7 || maxValGilgamesh < 0.7) && isPhase1Success) {
				//place yuno
				game.pressKey('3');
				game.mouseMove(1241, 567);
				game.leftClick();

				//place ichiko
				game.pressKey('2');
				game.mouseMove(1270, 569);
				game.leftClick();

				//place gilgamesh
				game.pressKey('1');
				game.mouseMove(1084, 700);
				game.leftClick();
			}

			//upgrade when found gilgamesh only once
			if (maxValGilgamesh > 0.8 && !isUpgrade) {
				
				//up1 
				game.mouseMove(1904, 425);
				game.leftClick();

				//up2 
				game.mouseMove(2096, 430);
				game.leftClick();

				//up5
				game.mouseMove(2092, 697);
				game.leftClick();
				game.leftClick();

				isUpgrade = true;
			}

			//retry
			if (maxValRetry > 0.5) {
				Sleep(200);
				//select 1 of 3 portal
				game.mouseMove(1283, 704);
				game.leftClick();
				Sleep(100);
				//confirm
				game.mouseMove(1286, 1084);
				game.leftClick();
				Sleep(200);
				//select 1 of 3 portal
				game.mouseMove(1283, 704);
				game.leftClick();
				Sleep(100);
				//confirm
				game.mouseMove(1286, 1084);
				game.leftClick();
				Sleep(100);
				//view portal
				game.mouseMove(1230, 940);
				game.leftClick();
				Sleep(200);
				//search portal
				game.mouseMove(926, 416);
				game.leftClick();
				game.pressKey('s');
				game.pressKey('u');
				Sleep(200);
				//choose first portal
				game.mouseMove(909, 556);
				game.leftClick();
				Sleep(200);
				//spawn portal
				game.mouseMove(1668, 1114);
				game.leftClick();
				Sleep(200);
				//enter portal
				game.mouseMove(1164, 791);
				game.leftClick();
			}
			else {
				//select 1 of 3 portal
				game.mouseMove(1283, 704);
				game.leftClick();

				//confirm
				game.mouseMove(1286, 1084);
				game.leftClick();
			}

			//nuke 904, 723
			if(maxValBoss > 0.15) {
				//click bulmy 
				game.mouseMove(2033, 336);
				game.leftClick();
				Sleep(100);
				//summon 908, 668
				game.mouseMove(908, 668);
				game.leftClick();
				//wealth
				Sleep(100);
				game.mouseMove(917, 674);
				game.leftClick();

				Sleep(200);
				game.mouseMove(2037, 609);
				game.leftClick();
				Sleep(200);
				game.mouseMove(904, 723);
				game.leftClick();
			}

		}

		while (true) {

		}

		//-=-=-=-=-EDIT ABOVE THIS-=-=-=-=-

	return 0;
}