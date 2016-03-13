#include "opencv/highgui.h"
#include "opencv/cv.h"
 
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))  
#define abs(x) ((x) > 0 ? (x) : -(x))
#define sign(x) ((x) > 0 ? 1 : -1)
 
#define STEP_MIN 5
#define STEP_MAX 100 

CvPoint binarisation(IplImage* image, int *nbPixels) ;
void addObjectToVideo(IplImage* image, CvPoint objectNextPos, int nbPixels);
void getObjectColor(int event, int x, int y, int flags, void *param);

IplImage *image;
 
CvPoint objectPos = cvPoint(-1, -1);
int h = 0, s = 0, v = 0, tolerance = 10;

int main() {
 
    IplImage *hsv;
    CvCapture *capture;
    char key;
 
    int nbPixels;
    CvPoint objectNextPos;
 
    capture = cvCreateFileCapture("/home/matthieu/Téléchargements/test.mp4");
 
        if (!capture) {
        printf("ERREUR ! impossible d'ouvrir le flux video\n");
            return -1;
    }
 
    cvNamedWindow("fenetre flux", CV_WINDOW_AUTOSIZE);
    cvNamedWindow("fenetre mask", CV_WINDOW_AUTOSIZE);
    cvMoveWindow("fenetre flux", 0, 100);
    cvMoveWindow("fenetre mask", 650, 100);
 
    /*on choisi la couleur à traquer en cliquant dessus*/
    cvSetMouseCallback("fenetre flux", getObjectColor);
 
    /*tant que l'on n'entre pas Q ou q les fenetre reste ouverte*/
    while(key != 'Q' && key != 'q') {
 
        image = cvQueryFrame(capture); /*chaque image de capture et stocké dans image*/
 
        if(!image)
            continue;
 
        /*création du mask, reperage de l'objet et du barycentre, estimation de la prochaine position*/
        objectNextPos = binarisation(image, &nbPixels);
        /*création du point sur le flux vidéo*/  
        addObjectToVideo(image, objectNextPos, nbPixels);
 
        key = cvWaitKey(10);
 
    }
    
    /*liberation de mémoire*/
    cvDestroyWindow("fenetre flux");
    cvDestroyWindow("fenetre mask");
 
    cvReleaseCapture(&capture);
 
    return 0;
 
}

CvPoint binarisation(IplImage* image, int *nbPixels) {
 
    int x, y;
    CvScalar pixel;
    IplImage *hsv, *mask;
    IplConvKernel *kernel;
    int sommeX = 0, sommeY = 0;
    *nbPixels = 0;
    
    /*créé un masque de la meme taille que nos images*/
    mask = cvCreateImage(cvGetSize(image), image->depth, 1);
 
    /*on fais une copie de image qu'on convertit en HSV*/
    hsv = cvCloneImage(image); 
    cvCvtColor(image, hsv, CV_BGR2HSV);
 
    /*on remplit le masque en annalysant notre copie de image en hsv*/
    /*on met les pixels appartenant à notre tolérence en blanc (255) sinon en noir (0) dans notre mask*/
    cvInRangeS(hsv, cvScalar(h - tolerance -1, s - tolerance, 0), cvScalar(h + tolerance -1, s + tolerance, 255), mask);
 
    /*on créé le noyau de l'operation morphologique, tas de taille 5*5 de forme ronde*/
    kernel = cvCreateStructuringElementEx(5, 5, 2, 2, CV_SHAPE_ELLIPSE);
    /*puis on dilate et erode notre objet*/
    cvDilate(mask, mask, kernel, 1); //renforce notre noyau
    cvErode(mask, mask, kernel, 1);  //supprime les taches indésirable
    
    /*on parcour le mask pour sommer les coordonnée des pixels qui nous interessent*/
    for(x = 0; x < mask->width; x++) {
        for(y = 0; y < mask->height; y++) { 
 
            if(((uchar *)(mask->imageData + y*mask->widthStep))[x] == 255) {
                sommeX += x;
                sommeY += y;
                (*nbPixels)++;
            }
        }
    }
 
    cvShowImage("fenetre mask", mask);
 
    cvReleaseStructuringElement(&kernel);
 
    cvReleaseImage(&mask);
        cvReleaseImage(&hsv);
    /*si il y a des pixels qui nous interesse, on calcul le barycentre et on revoit ses
      coordonnées. si il n'y a pas de pixel qui nous interesse on renvoi le point (-1;-1)*/
    if(*nbPixels > 0)
        return cvPoint((int)(sommeX / (*nbPixels)), (int)(sommeY / (*nbPixels)));
    else
        return cvPoint(-1, -1);
}
 



void addObjectToVideo(IplImage* image, CvPoint objectNextPos, int nbPixels) {
 
    int objectNextStepX, objectNextStepY;
    
    /*on lisse le deplacement*/
    if (nbPixels > 10) {
        /*objectPos : position actuel   
          objectNextPos : futur position basé sur le calcul du barycentre*/
        if (objectPos.x == -1 || objectPos.y == -1) {
            objectPos.x = objectNextPos.x;
            objectPos.y = objectNextPos.y;
        }
 
        if (abs(objectPos.x - objectNextPos.x) > STEP_MIN) {
            objectNextStepX = max(STEP_MIN, min(STEP_MAX, abs(objectPos.x - objectNextPos.x) / 2));
            objectPos.x += (-1) * sign(objectPos.x - objectNextPos.x) * objectNextStepX;
        }
        if (abs(objectPos.y - objectNextPos.y) > STEP_MIN) {
            objectNextStepY = max(STEP_MIN, min(STEP_MAX, abs(objectPos.y - objectNextPos.y) / 2));
            objectPos.y += (-1) * sign(objectPos.y - objectNextPos.y) * objectNextStepY;
        }
 
    } else {
 
        objectPos.x = -1;
        objectPos.y = -1;
 
    }
     /*au dela de 10 pixels trouvé nous dessinons un cercle centré sur le barycentre*/
    if (nbPixels > 10)
        cvDrawCircle(image, objectPos, 15, CV_RGB(255, 0, 0), -1);
 
    cvShowImage("fenetre flux", image);
 
}
 



void getObjectColor(int event, int x, int y, int flags, void *param) {
 
    CvScalar pixel;
    IplImage *hsv;
    
    /*si on fait un clic gauche*/
    if(event == CV_EVENT_LBUTTONUP) {
         /*on convertit l'image en hsv*/
        hsv = cvCloneImage(image);
        cvCvtColor(image, hsv, CV_BGR2HSV);
        
        /*on récupère les données du pixel selectionné*/
        pixel = cvGet2D(hsv, y, x);
        
        /*on met a jour nos donné hsv*/
        h = (int)pixel.val[0];
        s = (int)pixel.val[1];
        v = (int)pixel.val[2];
 
        cvReleaseImage(&hsv);
 
    }
 
}