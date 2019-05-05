/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenALPR.
 *
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include <numeric>      // std::accumulate

#include "alpr_impl.h"

#include "endtoendtest.h"

#include "detection/detectorfactory.h"
#include "ocr/ocrfactory.h"
#include "support/filesystem.h"

using namespace std;
using namespace cv;
using namespace alpr;

// Given a directory full of lp images (named [statecode]#.png) crop out the alphanumeric characters.
// These will be used to train the OCR

void outputStats(vector<double> datapoints);





int main( int argc, const char** argv )
{
  string country;
  string benchmarkName;
  string inDir;
  string outDir;
  Mat frame;
    //Mat bound_box;

  //Check if user specify image to process
  if(argc == 5)
  {
    country = argv[1];
    benchmarkName = argv[2];
    inDir = argv[3];
    outDir = argv[4];
  }
  else
  {
    printf("Use:\n\t%s [country] [benchmark name] [img input dir] [results output dir]\n",argv[0]);
    printf("\tex: %s us speed ./speed/usimages ./speed\n",argv[0]);
    printf("\n");
    printf("\ttest names are: speed, segocr, detection\n\n" );
    return 0;
  }

  if (DirectoryExists(inDir.c_str()) == false)
  {
    printf("Input dir does not exist\n");
    return 0;
  }
  if (DirectoryExists(outDir.c_str()) == false)
  {
    printf("Output dir does not exist\n");
    return 0;
  }

  vector<string> files = getFilesInDir(inDir.c_str());
  sort( files.begin(), files.end(), stringCompare );

  if (benchmarkName.compare("segocr") == 0)
  {
    Config* config = new Config(country);
    config->setDebug(false);
    config->skipDetection = true;

    AlprImpl alpr(country);
    alpr.config = config;
    
    for (int i = 0; i< files.size(); i++)
    {
      if (hasEnding(files[i], ".png") || hasEnding(files[i], ".jpg"))
      {
        string fullpath = inDir + "/" + files[i];

        frame = cv::imread(fullpath.c_str());
        
        cv::Rect roi;
        roi.x = 0;
        roi.y = 0;
        roi.width = frame.cols;
        roi.height = frame.rows;
        vector<Rect> rois;
        rois.push_back(roi);
        AlprResults results = alpr.recognize(frame, rois);
        
        char statecode[3];
        statecode[0] = files[i][0];
        statecode[1] = files[i][1];
        statecode[2] = '\0';
        string statecodestr(statecode);
                
        if (results.plates.size() == 1)
          cout << files[i] << "," << statecode << "," << results.plates[0].bestPlate.characters << endl;
        else if (results.plates.size() == 0)
          cout << files[i] << "," << statecode << "," << endl;
        else if (results.plates.size() > 1)
          cout << files[i] << "," << statecode << ",???+" << endl;

        imshow("Current LP", frame);
          cout << "segocr" << endl;
        waitKey(5);
      }
    }

    delete config;
  }
  else if (benchmarkName.compare("detection") == 0)
  {
    Config config(country);
    PreWarp prewarp(&config);
    Detector* plateDetector = createDetector(&config, &prewarp);

    for (int i = 0; i< files.size(); i++)
    {
      if (hasEnding(files[i], ".png") || hasEnding(files[i], ".jpg"))
      {
        string fullpath = inDir + "/" + files[i];
        frame = imread( fullpath.c_str() );

        vector<PlateRegion> regions = plateDetector->detect(frame);

        imshow("Current LP", frame);
        waitKey(5);
      }
    }
    
    delete plateDetector;
  }
  else if (benchmarkName.compare("speed") == 0)
  {
    // Benchmarks speed of region detection, plate analysis, and OCR

    timespec startTime;
    timespec endTime;

    Config config(country);
    config.setDebug(false);

    config.skipDetection = true;

    AlprImpl alpr(country);
    alpr.config->setDebug(false);
    alpr.setDetectRegion(true);

    PreWarp prewarp(&config);
    Detector* plateDetector = createDetector(&config, &prewarp);
    OCR* ocr = createOcr(&config);

    vector<double> endToEndTimes;
    vector<double> regionDetectionTimes;
    vector<double> stateIdTimes;
    vector<double> lpAnalysisPositiveTimes;
    vector<double> lpAnalysisNegativeTimes;
    vector<double> ocrTimes;
    vector<double> postProcessTimes;

    for (int i = 0; i< files.size(); i++)
    {
      if (hasEnding(files[i], ".png") || hasEnding(files[i], ".jpg"))
      {
        cout << "Image: " << files[i] << endl;

        string fullpath = inDir + "/" + files[i];
        frame = imread( fullpath.c_str() );

        getTimeMonotonic(&startTime);
        vector<Rect> regionsOfInterest;

        regionsOfInterest.push_back(Rect(0, 0, frame.cols, frame.rows));

        //teddyxu: feed with plate coordinates
        //regionsOfInterest.push_back(Rect(238,355,121,60));
        //regionsOfInterest.push_back(Rect(193,294,109,54));
        //regionsOfInterest.push_back(Rect(156,235,90,45));
        //regionsOfInterest.push_back(Rect(113,173,77,39));

        //teddyxu: feed with plate coordinates
        //regionsOfInterest.push_back(Rect(682,205,129,65));
        //regionsOfInterest.push_back(Rect(509,151,104,52));
        //regionsOfInterest.push_back(Rect(330,97,86,43));
        //regionsOfInterest.push_back(Rect(113,173,77,39));

        AlprResults myresult = alpr.recognize(frame, regionsOfInterest);

        getTimeMonotonic(&endTime);
        double endToEndTime = diffclock(startTime, endTime);
        cout << " -- End to End recognition time: " << endToEndTime << "ms." << endl;
        endToEndTimes.push_back(endToEndTime);

        std::cout << alpr.toJson( myresult ) << std::endl;

        getTimeMonotonic(&startTime);
        vector<PlateRegion> regions = plateDetector->detect(frame);

#if 0
          //teddyxu
          vector<PlateRegion> regions;
          PlateRegion fake;
          fake.rect.x = 327; //643,459,327,160
          fake.rect.y = 103; //190,140,103,48
          fake.rect.width = 193; //395,276,193,99
          fake.rect.height = 48; //99,69,48,25

          regions.push_back(fake);
          vector<PlateRegion> regions = plateDetector->detect(bound_box);
          regions.push_back(&bound_box);
#endif
        getTimeMonotonic(&endTime);

        double regionDetectionTime = diffclock(startTime, endTime);
        cout << " -- Region detection time: " << regionDetectionTime << "ms." << endl;
        regionDetectionTimes.push_back(regionDetectionTime);

        //teddyxu
        cout << "regions size " << regions.size() << endl;


        for (int z = 0; z < regions.size(); z++)
        {
            PipelineData pipeline_data(frame, regions[z].rect, &config);
            getTimeMonotonic(&startTime);


          //stateDetector.detect(&pipeline_data);
          getTimeMonotonic(&endTime);
          double stateidTime = diffclock(startTime, endTime);
          cout << "\tRegion " << z << ": State ID time: " << stateidTime << "ms." << endl;
          stateIdTimes.push_back(stateidTime);

          getTimeMonotonic(&startTime);
          LicensePlateCandidate lp(&pipeline_data);
          lp.recognize();
          getTimeMonotonic(&endTime);
          double analysisTime = diffclock(startTime, endTime);
          cout << "\tRegion " << z << ": Analysis time: " << analysisTime << "ms." << endl;

            // teddyxu changed:
          if (pipeline_data.disqualified)
              //if (!pipeline_data.disqualified)
          {
            lpAnalysisPositiveTimes.push_back(analysisTime);

            getTimeMonotonic(&startTime);
            ocr->performOCR(&pipeline_data);
            getTimeMonotonic(&endTime);
            double ocrTime = diffclock(startTime, endTime);
            cout << "\tRegion " << z << ": OCR time: " << ocrTime << "ms." << endl;
            ocrTimes.push_back(ocrTime);

            getTimeMonotonic(&startTime);
            ocr->postProcessor.analyze("", 25);
            getTimeMonotonic(&endTime);
            double postProcessTime = diffclock(startTime, endTime);
            cout << "\tRegion " << z << ": PostProcess time: " << postProcessTime << "ms." << endl;
            cout << "ocr res" << ocr->postProcessor.bestChars << endl;

            postProcessTimes.push_back(postProcessTime);
          }
          else
          {
            lpAnalysisNegativeTimes.push_back(analysisTime);
          }
        }

        waitKey(5);
      }
    }

    cout << endl << "---------------------" << endl;

    cout << "End to End Time Statistics:" << endl;
    outputStats(endToEndTimes);
    cout << endl;

    cout << "Region Detection Time Statistics:" << endl;
    outputStats(regionDetectionTimes);
    cout << endl;

    cout << "State ID Time Statistics:" << endl;
    outputStats(stateIdTimes);
    cout << endl;

    cout << "Positive Region Analysis Time Statistics:" << endl;
    outputStats(lpAnalysisPositiveTimes);
    cout << endl;

    cout << "Negative Region Analysis Time Statistics:" << endl;
    outputStats(lpAnalysisNegativeTimes);
    cout << endl;

    cout << "OCR Time Statistics:" << endl;
    outputStats(ocrTimes);
    cout << endl;

    cout << "Post Processing Time Statistics:" << endl;
    outputStats(postProcessTimes);
    cout << endl;
  }
  else if (benchmarkName.compare("endtoend") == 0)
  {
    EndToEndTest e2eTest(inDir, outDir);
    e2eTest.runTest(country, files);
    
  }
}

void outputStats(vector<double> datapoints)
{
  double sum = std::accumulate(datapoints.begin(), datapoints.end(), 0.0);
  double mean = sum / datapoints.size();

  std::vector<double> diff(datapoints.size());
  std::transform(datapoints.begin(), datapoints.end(), diff.begin(),
                 std::bind2nd(std::minus<double>(), mean));
  double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double stdev = std::sqrt(sq_sum / datapoints.size());

  cout << "\t" << datapoints.size() << " samples, avg: " << mean << "ms,  stdev: " << stdev << endl;
}
