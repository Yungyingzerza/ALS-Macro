
#include "GameManager.h"

int main()
{

    GameManager game;
    cv::Mat frame;

    //-=-=-=-=-EDIT BELOW THIS-=-=-=-=-

    cv::Rect towersRegion(1800, 200, 650, 200);
    cv::Rect retryRegion(800, 850, 950, 150);// 1735, 989
    cv::Rect startRegion(950, 1050, 350, 150); //1278, 1150
    cv::Mat phanse1Image = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\phase1.png", cv::IMREAD_COLOR);
    cv::Mat phanse11Image = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\phase11.png", cv::IMREAD_COLOR);
    cv::Mat retryImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\retry.png", cv::IMREAD_COLOR);
    cv::Mat startImage = cv::imread("C:\\Users\\weera\\source\\repos\\AnimeLastStand\\x64\\Release\\start.png", cv::IMREAD_COLOR);

    // Check if the image was successfully loaded
    if (phanse1Image.empty() || phanse11Image.empty() || retryImage.empty() || startImage.empty()) {
        std::cerr << "Error: Could not load Image." << std::endl;
        return -1;
    }

    //-=-=-=-=-EDIT ABOVE THIS-=-=-=-=-=-

	while (game.isRunning()) {

		while (game.isPaused()) {
			Sleep(100);
		}

		//-=-=-=-=-EDIT BELOW THIS-=-=-=-=-

		bool isFirst = true;

		//check start
		while (true) {
			game.Capture(frame);

			if (frame.empty()) break;

			cv::Mat towers = frame(startRegion);
			if (towers.empty()) break;

			cv::cvtColor(towers, towers, cv::COLOR_BGRA2BGR);

			cv::Mat result;
			cv::matchTemplate(towers, startImage, result, cv::TM_CCOEFF_NORMED);

			if (result.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			double minVal, maxVal;
			cv::minMaxLoc(result, &minVal, &maxVal);

			//have all require towers
			if (maxVal > 0.5) {
				Sleep(200);

				std::cout << "Val: " << maxVal << '\n';

				game.mouseScroll(true);
				game.panCameraRight(200);

				Sleep(2000);

				break;
			}

		}

		//check phase1
		while (true) {
			game.Capture(frame);

			if (frame.empty()) break;

			cv::Mat towers = frame(towersRegion);
			if (towers.empty()) break;

			cv::cvtColor(towers, towers, cv::COLOR_BGRA2BGR);

			cv::Mat result, result2;
			cv::matchTemplate(towers, phanse1Image, result, cv::TM_CCOEFF_NORMED);
			cv::matchTemplate(towers, phanse11Image, result2, cv::TM_CCOEFF_NORMED);

			if (result.empty() || result2.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			double minVal, minVal2, maxVal, maxVal2;
			cv::minMaxLoc(result, &minVal, &maxVal);
			cv::minMaxLoc(result2, &minVal2, &maxVal2);

			//have all require towers
			if (maxVal > 0.5 || maxVal2 > 0.5) {
				Sleep(200);

				//unit manager
				game.mouseMove(2388, 769);
				game.leftClick();

				//up1 
				game.mouseMove(1999, 430);
				game.leftClick();

				//up2 
				game.mouseMove(2185, 427);
				game.leftClick();

				//up3 
				game.mouseMove(2385, 426);
				game.leftClick();

				break;
			}
			else {
				//start
				game.mouseMove(1143, 1115);
				game.leftClick();

				game.mouseScroll(true);

				game.pressKey('4');
				game.mouseMove(1054, 646);
				game.leftClick();

				game.pressKey('3');
				game.mouseMove(1241, 567);
				game.leftClick();

				game.pressKey('2');
				game.mouseMove(1270, 569);
				game.leftClick();

				//unit manager
				game.mouseMove(2388, 769);
				game.leftClick();
			}
		}

		//check retry
		while (true) {
			game.Capture(frame);

			if (frame.empty()) break;

			cv::Mat towers = frame(retryRegion);
			if (towers.empty()) break;

			cv::cvtColor(towers, towers, cv::COLOR_BGRA2BGR);

			cv::Mat result;
			cv::matchTemplate(towers, retryImage, result, cv::TM_CCOEFF_NORMED);

			if (result.empty()) {
				std::cerr << "Error: matchTemplate failed.\n";
				continue;
			}

			double minVal, maxVal;
			cv::minMaxLoc(result, &minVal, &maxVal);

			//found button
			if (maxVal > 0.5) {
				Sleep(1000);

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

				Sleep(100);

				//choose portal
				game.mouseMove(1194, 709);
				game.leftClick();

				Sleep(100);

				//spawn portal
				game.mouseMove(1668, 1114);
				game.leftClick();

				Sleep(100);

				//enter portal
				game.mouseMove(1164, 791);
				game.leftClick();

				break;
			}
			else {

				if (isFirst) {
					//select idol(1) 
					game.mouseMove(1939, 335);
					game.leftClick();

					//use skill auto 
					game.mouseMove(1045, 657);
					game.leftClick();

					//select ichiko(2) 
					game.mouseMove(2131, 337);
					game.leftClick();

					//use skill auto 
					game.mouseMove(1045, 657);
					game.leftClick();

					//select ichiko(3) 
					game.mouseMove(2332, 358);
					game.leftClick();

					//use skill auto 
					game.mouseMove(1045, 657);
					game.leftClick();

					isFirst = false;
				}
				else {
					//select 1 of 3 portal
					game.mouseMove(1283, 704);
					game.leftClick();

					//confirm
					game.mouseMove(1286, 1084);
					game.leftClick();

					game.pressKey('6');
					game.mouseMove(1650, 838);
					game.leftClick();

					game.pressKey('1');
					game.mouseMove(1245, 585);
					game.leftClick();

					/*pressKey('5');
					mouseMove(1267, 608);
					leftClick();*/

					//up4
					game.mouseMove(2480, 433);
					game.leftClick();

					//up5
					game.mouseMove(2092, 697);
					game.leftClick();

				}

			}

		}

		//-=-=-=-=-EDIT ABOVE THIS-=-=-=-=-

	}

    return 0;

}
