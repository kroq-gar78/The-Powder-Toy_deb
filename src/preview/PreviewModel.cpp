/*
 * PreviewModel.cpp
 *
 *  Created on: Jan 21, 2012
 *      Author: Simon
 */

#include "PreviewModel.h"
#include "client/Client.h"
#include "PreviewModelException.h"

PreviewModel::PreviewModel():
	save(NULL),
	saveComments(NULL),
	doOpen(false),
	updateSaveDataWorking(false),
	updateSaveDataFinished(false),
	updateSaveInfoWorking(false),
	updateSaveInfoFinished(false),
	updateSaveCommentsWorking(false),
	updateSaveCommentsFinished(false)
{
	// TODO Auto-generated constructor stub

}

void * PreviewModel::updateSaveInfoTHelper(void * obj)
{
	return ((PreviewModel*)obj)->updateSaveInfoT();
}

void * PreviewModel::updateSaveDataTHelper(void * obj)
{
	return ((PreviewModel*)obj)->updateSaveDataT();
}

void * PreviewModel::updateSaveCommentsTHelper(void * obj)
{
	return ((PreviewModel*)obj)->updateSaveCommentsT();
}

void * PreviewModel::updateSaveInfoT()
{
	SaveInfo * tempSave = Client::Ref().GetSave(tSaveID, tSaveDate);
	updateSaveInfoFinished = true;
	return tempSave;
}

void * PreviewModel::updateSaveDataT()
{
	int tempDataSize;
	unsigned char * tempData = Client::Ref().GetSaveData(tSaveID, tSaveDate, tempDataSize);
	saveDataBuffer.clear();
	saveDataBuffer.insert(saveDataBuffer.begin(), tempData, tempData+tempDataSize);
	updateSaveDataFinished = true;
	return NULL;
}

void * PreviewModel::updateSaveCommentsT()
{
	std::vector<SaveComment*> * tempComments = Client::Ref().GetComments(tSaveID, 0, 10);
	updateSaveCommentsFinished = true;
	return tempComments;
}

void PreviewModel::SetFavourite(bool favourite)
{
	//if(save)
	{
		Client::Ref().FavouriteSave(save->id, favourite);
		save->Favourite = favourite;
		notifySaveChanged();
	}
}

void PreviewModel::UpdateSave(int saveID, int saveDate)
{
	this->tSaveID = saveID;
	this->tSaveDate = saveDate;

	if(save)
	{
		delete save;
		save = NULL;
	}
	saveDataBuffer.clear();
	if(saveComments)
	{
		for(int i = 0; i < saveComments->size(); i++)
			delete saveComments->at(i);
		delete saveComments;
		saveComments = NULL;
	}
	notifySaveChanged();
	notifySaveCommentsChanged();

	if(!updateSaveDataWorking)
	{
		updateSaveDataWorking = true;
		updateSaveDataFinished = false;
		pthread_create(&updateSaveDataThread, 0, &PreviewModel::updateSaveDataTHelper, this);
	}

	if(!updateSaveInfoWorking)
	{
		updateSaveInfoWorking = true;
		updateSaveInfoFinished = false;
		pthread_create(&updateSaveInfoThread, 0, &PreviewModel::updateSaveInfoTHelper, this);
	}

	if(!updateSaveCommentsWorking)
	{
		updateSaveCommentsWorking = true;
		updateSaveCommentsFinished = false;
		pthread_create(&updateSaveCommentsThread, 0, &PreviewModel::updateSaveCommentsTHelper, this);
	}
}

void PreviewModel::SetDoOpen(bool doOpen)
{
	this->doOpen = doOpen;
}

bool PreviewModel::GetDoOpen()
{
	return doOpen;
}

SaveInfo * PreviewModel::GetSave()
{
	return save;
}

std::vector<SaveComment*> * PreviewModel::GetComments()
{
	return saveComments;
}

void PreviewModel::notifySaveChanged()
{
	for(int i = 0; i < observers.size(); i++)
	{
		observers[i]->NotifySaveChanged(this);
	}
}

void PreviewModel::notifySaveCommentsChanged()
{
	for(int i = 0; i < observers.size(); i++)
	{
		observers[i]->NotifyCommentsChanged(this);
	}
}

void PreviewModel::AddObserver(PreviewView * observer) {
	observers.push_back(observer);
	observer->NotifySaveChanged(this);
}

void PreviewModel::Update()
{
	if(updateSaveDataWorking)
	{
		if(updateSaveDataFinished)
		{
			updateSaveDataWorking = false;
			pthread_join(updateSaveDataThread, NULL);

			if(updateSaveInfoFinished && save)
			{
				try
				{
					save->SetGameSave(new GameSave(&saveDataBuffer[0], saveDataBuffer.size()));
				}
				catch(ParseException &e)
				{
					throw PreviewModelException("Save file corrupt or from newer version");
				}
				notifySaveChanged();
			}
		}
	}

	if(updateSaveInfoWorking)
	{
		if(updateSaveInfoFinished)
		{
			if(save)
			{
				delete save;
				save = NULL;
			}
			updateSaveInfoWorking = false;
			pthread_join(updateSaveInfoThread, (void**)(&save));
			if(updateSaveDataFinished && save)
			{
				try
				{
					save->SetGameSave(new GameSave(&saveDataBuffer[0], saveDataBuffer.size()));
				}
				catch(ParseException &e)
				{
					throw PreviewModelException("Save file corrupt or from newer version");
				}
			}
			notifySaveChanged();
			if(!save)
				throw PreviewModelException("Unable to load save");
		}
	}

	if(updateSaveCommentsWorking)
	{
		if(updateSaveCommentsFinished)
		{
			if(saveComments)
			{
				for(int i = 0; i < saveComments->size(); i++)
					delete saveComments->at(i);
				delete saveComments;
				saveComments = NULL;
			}
			updateSaveCommentsWorking = false;
			pthread_join(updateSaveCommentsThread, (void**)(&saveComments));
			notifySaveCommentsChanged();
		}
	}
}

PreviewModel::~PreviewModel() {
	if(save)
		delete save;
}
